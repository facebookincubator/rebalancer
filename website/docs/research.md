---
sidebar_position: 100
---

# Research Papers

## Optimizing Resource Allocation in Hyperscale Datacenters: Scalability, Usability, and Experiences

**Authors**: Neeraj Kumar, Pol Mauri Ruiz, Vijay Menon, Igor Kabiljo, Mayank Pundir, Andrew Newell, Daniel Lee, Liyuan Wang, and Chunqiang Tang

**Conference**: OSDI 2024

**PDF**: [PDF](https://www.usenix.org/system/files/osdi24-kumar.pdf)

**Abstract**: Meta’s private cloud uses millions of servers to host tens of thousands of services that power multiple products for billions of users. This complex environment has various optimization problems involving resource allocation, including hardware placement, server allocation, ML training & inference placement, traffic routing, database & container migration for load balancing, grouping serverless functions for locality, etc. The main challenges for a reusable resource-allocation framework are its usability and scalability. Usability is impeded by practitioners struggling to translate real-life policies into precise mathematical formulas required by formal optimization methods, while scalability is hampered by NPhard problems that cannot be solved efficiently by commercial solvers. These challenges are addressed by Rebalancer, Meta’s resource-allocation framework. It has been applied to dozens of large-scale use cases over the past seven years, demonstrating its usability, scalability, and generality. At the core of Rebalancer is an expression graph that enables its optimization algorithm to run more efficiently than past algorithms. Moreover, Rebalancer offers a high-level specification language to lower the barrier for adoption by systems practitioners.

**BibTeX**:
```bibtex
@inproceedings{kumar2024optimizing,
  title={Optimizing resource allocation in hyperscale datacenters: Scalability, usability, and experiences},
  author={Kumar, Neeraj and Ruiz, Pol Mauri and Menon, Vijay and Kabiljo, Igor and Pundir, Mayank and Newell, Andrew and Lee, Daniel and Wang, Liyuan and Tang, Chunqiang},
  booktitle={18th USENIX Symposium on Operating Systems Design and Implementation (OSDI 24)},
  pages={507--528},
  year={2024}
}
```

## Contact

For questions about the research or academic collaboration please open a GitHub issue or contact:
* nks@meta.com
* rbarnes@meta.com
* lakshmiganesh@meta.com

