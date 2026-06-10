# Documentation Guidelines for MoveTypes

This document provides guidelines for creating and maintaining documentation for MoveTypes in the ReBalancer solver.

## Documentation Structure

Each concrete MoveType (non-abstract classes) should have exactly one corresponding Markdown file in the `docs/` directory. The documentation file should follow this naming convention:
- Header file: `SomeExampleMoveType.h`
- Documentation file: `docs/Some_Example.md`

## Documentation Format

Each MoveType documentation file must follow this consistent structure:

### 1. Name Section
```markdown
**Name:** MOVE_TYPE_CONSTANT_NAME
```
Start with the constant name used in the code (e.g., `kSingleChainMoveTypeName`).

### 2. Brief Description
Provide a concise one-sentence description of what the move type evaluates or accomplishes. This requires understanding the innerworking of corresponding MoveType.cpp. Start with findBestMove function.

### 3. Parameters Section
```markdown
## **Parameters**
* **parameterName:** description of what this parameter controls, including default values and behavior when unset.
```
Document all configurable parameters from the MoveType specification. Include:
- Parameter name exactly as it appears in the spec
- Clear description of functionality
- Default values or behavior when unset
- Any constraints or valid ranges
- Pick the parameters from SolveSpecs.thrift and appropriate configs

### 4. Behavior Section
```markdown
## **Behavior**
```
Describe the algorithmic behavior step-by-step:
1. Start with reference to "common logic" for hot container selection
2. Provide numbered steps explaining the move generation process
3. Explain how combinations are evaluated
4. Describe the selection criteria for the final move

### 5. Complexity Section
```markdown
## **Complexity**
```
Provide the approximate number of neighboring assignments explored, using Big O notation or concrete formulas in terms of:
- `objects` - number of objects in containers
- `containers` - number of containers
- Other relevant parameters

### 6. Diagram Section (Optional)
```markdown
## **Diagram**
![](/intern/wiki/_download/?title=MoveTypeName.png)
```
Include visual diagrams when available to illustrate the move pattern.

## Documentation Update Rules

Documentation should only be updated when:
1. **Parameters change**: New parameters added, existing parameters modified, or parameters removed
2. **Move generation logic changes**: The algorithm for generating or exploring moves is modified
3. **Complexity changes**: The computational complexity or number of explored assignments changes

Do NOT update documentation for:
- Code refactoring that doesn't change behavior
- Performance optimizations that don't affect complexity
- Internal implementation details that don't affect the external interface

## Best Practices

1. **Consistency**: Use consistent terminology across all MoveType documentation
2. **Clarity**: Write for users who need to understand the behavior and performance characteristics
3. **Precision**: Use exact parameter names and values from the code
4. **Completeness**: Cover all configurable parameters and their effects
5. **Accuracy**: Ensure complexity estimates match the actual implementation

## Abstract Classes

Do NOT create documentation for abstract base classes such as:
- `MoveType.h`
- `AsyncSingleMovesMoveType.h`
- `AsyncDestinationsMoveType.h`

Only document concrete MoveType implementations that can be instantiated and used directly.

## File Maintenance

- Use the exact file paths when referencing documentation files
- Maintain the CFM_RULES file to track documentation dependencies
- Follow the existing naming conventions for consistency
- Keep cross-references up-to-date when MoveTypes are renamed or removed

## Links and References

- Reference the common logic documentation for hot container selection
- Link to related MoveTypes when appropriate (e.g., comparing Single Chain vs Single End Chain)
- Use internal wiki links for diagrams and additional resources

This documentation serves as the single source of truth for MoveType behavior and should be updated whenever the underlying implementation changes in ways that affect user-visible behavior or performance characteristics.
