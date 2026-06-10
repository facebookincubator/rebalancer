#!/usr/bin/env python3
# Copyright (c) Meta Platforms, Inc. and affiliates.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.


# pyre-strict

from collections.abc import Callable

from algopt.rebalancer.common.host_data import HostData
from algopt.rebalancer.common.rack_data import RackData
from algopt.rebalancer.common.utils import checkState
from facebook.core_systems.cluster.thrift_types import ClusterType
from libfb.py import hostnameutils


TYPE_TO_COST_MAP: dict[str, float] = {
    "IVY BRIDGE T1": 3.2,
    "BROADWELL T1": 2.58,
    "BROADWELL DE T1": 2.58,
    "SKYLAKE T1": 3.25,
    "SKYLAKE DE T1": 3.25,
    "HASWELL T1": 4.2,
    "BROADWELL T6": 5.1,
    "BROADWELL DE T6": 5.1,
}


class CannotExtractHostScheme(Exception):
    pass


class HostIsNotInFBUQDN(Exception):
    pass


def clean_host(host: str, serf: dict[str, ClusterType] | None = None) -> str:
    """
    Converts the host to a FB UQDN and guarantees it is valid.

    host: The string hostname to clean
    serf: The optional serf dict associated with the given hostname

    returns: The cleaned string hostname
    """
    host = hostnameutils.to_fb_uqdn(host)
    # Throws if basic checks don't pass.
    host_to_scheme(host, serf)
    return str(host)


def host_with_facebook(host: str) -> str:
    """
    Adds a facebook.com suffix to an FB UQDN.

    host: A string hostname to modify

    returns: An FB FQDN with the proper suffix (facebook.com)
    """
    assert_cleaned_host(host)
    return hostnameutils.to_fb_fqdn(host)


def assert_cleaned_host(host: str) -> None:
    """
    Raises an exception if the given host is not already an FB UQDN

    host: The hostname to check
    """
    if hostnameutils.to_fb_uqdn(host) != host:
        raise HostIsNotInFBUQDN(hostnameutils.to_fb_uqdn(host) + " != " + host)
    # Throws if we don't know how to extract hostscheme.
    host_to_scheme(host)


def right_strip_numbers(value: str) -> str:
    return value.rstrip("0123456789")


def is_fake_host(hostname: str) -> bool:
    return "to__be__allocated" in hostname


def fake_host_to_request_id(fake_host: str) -> int:
    checkState(is_fake_host(fake_host), f"{fake_host} not fake host")
    # TODO T56120752: deprecate these usages
    return int(fake_host.split("__")[0].split("|")[1])


# TODO: Create a host abstraction and hide properties of
# the host behind the abstraction
# Also serf_data will be Union of all enums so remove Any.
def host_to_scheme(host: str, serf: dict[str, ClusterType] | None = None) -> str:
    """
    Determines the scheme for the given hostname. Additional serf information
    may optionally be provided, but it is required for IT and FACNET cluster
    hosts.

    host: The hostname to get the scheme of
    serf: An optional Dict of serf data associated with the given host

    returns: The scheme for the given host.
    """

    # Fake hosts added in allocation_usecase have artificial names:
    # the format is "scheme"|"server_type"__"server_type"__to__be__allocated__id"
    # e.g. "testscheme|1__1__to__be__allocated__2"
    # or "testscheme::some.svc.hostgroup|1__1__to__be__allocated__1"
    if is_fake_host(host):
        return host.split("__")[0].split("::")[0].split("|")[0]

    parts: list[str] = host.split(".")
    if len(parts) != 3 and len(parts) != 2:
        # Special handling of hosts in IT and FACNET clusters.
        if serf:
            if serf.get("cluster_obj.type", None) in [
                ClusterType.IT,
                ClusterType.FACNET,
            ]:
                return serf["cluster_obj.type"].name.lower() + "_cluster"
            else:
                raise CannotExtractHostScheme(
                    "Cannot extract hostscheme from " + str(host)
                )
        return hostnameutils.host_prefix(host)
    return right_strip_numbers(parts[0])


def host_to_region(host: str) -> str:
    """
    Gets the region string from a given hostname.

    host: The hostname to gt the region from

    returns: The region of the host
    """
    assert_cleaned_host(host)
    return right_strip_numbers(host.split(".")[-1])


def host_is_in_dcs_filter(datacenters: list[str]) -> Callable[[str], bool]:
    """
    Gets a lambda that filters hosts down to those only in the given list of
    datacenters.

    datacenters: A List of datacenter names the host might be in

    returns: A lambda that returns true if the hostname passed is in one of the
             given datacenters
    """
    return lambda host: any(host.endswith("." + dc) for dc in datacenters)


def host_is_in_region_filter(region: str | set[str]) -> Callable[[str], bool]:
    """
    Gets a lambda that determines if a host is in the given region or set of
    regions.

    region: A string or Set of strings to check

    returns: A lambda that returns true if the hostname passed is in the region
             or in any of the regions in the set
    """
    if isinstance(region, str):
        return lambda host: right_strip_numbers(host).endswith("." + str(region))
    else:
        return lambda host: right_strip_numbers(host).rsplit(".", 1)[-1] in region


def tier_is_in_region_filter(region: str) -> Callable[[list[str]], bool]:
    """
    Gets a lambda that determines if a tier is in the given region.

    region: A string to check against

    returns: A lambda that will check if the given region string is contained in
             the given tier string
    """
    return lambda tier: region in tier


def get_cpu_architecture(host_data: HostData) -> str:
    """Gets the CPU architecture from the given host's serf data."""
    return host_data.serf["part_obj.server_part_details.cpu_architecture"]


def get_server_type(host_data: HostData) -> str:
    """Gets the server type from the given host's serf data."""
    return host_data.serf["part_obj.server_part_details.server_type"]


def get_distinct_dc_and_suites(
    containers_data: dict[str, RackData] | dict[str, HostData],
) -> list[tuple[str, str]]:
    """
    Retrieves a distinct list of (datacenter, suite) pairs from the given hosts data
    or racks data. Returns distinct list[(datacenter, suite)].
    """
    return sorted(
        {
            (data.serf["datacenter"], data.serf["suite"])
            for data in containers_data.values()
            if not data.to_allocate
        }
    )


def host_name_to_server_type(host_name: str) -> str:
    """
    Example prefix1|1__1__to__be__allocated__0
    or prefix2\\:\\:group+a|6__6__to__be__allocated__0
    or prefix1|REQUESTID__1__to__be__allocated__0
    """
    if "to__be__allocated" in host_name:
        return host_name.split("__")[1]

    raise Exception(f"Cannot determine server type from name {host_name}")


def derive_rack_data_from_host_data(
    host_datas: list[HostData],
) -> dict[str, RackData]:
    result: dict[str, RackData] = {}

    for host_data in host_datas:
        if host_data.rack in result:
            result[host_data.rack].hosts.append(host_data.name)
            continue

        rack_data = RackData(
            name=host_data.rack,
            not_accepting=False,
            to_allocate=False,
            hosts=[host_data.name],
        )

        host_serf = (
            host_data.serf if not host_data.serf_to_match else host_data.serf_to_match
        )

        if "rack_serial" in host_serf and host_serf["rack_serial"].isdigit() is True:
            rack_data.rack_serial = int(host_serf["rack_serial"])

        dc = host_serf["datacenter"]
        cluster = host_serf["cluster"]
        if dc:
            rack_data.serf["region"] = dc[0:3]
            if cluster:
                rack_data.serf["dc_cluster"] = dc + "." + cluster

        rack_data.serf["datacenter"] = dc
        rack_data.serf["pod"] = host_serf["pod"]
        rack_data.serf["suite"] = host_serf["suite"]
        rack_data.serf["cluster"] = host_serf["cluster"]
        rack_data.serf["row"] = host_serf["row"]

        result[rack_data.name] = rack_data

    return result
