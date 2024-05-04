# WLP4 Compiler

This C++ programs compile WLP4 code into MIPS assembly language. WLP4 is a subset of the C programming language with some simplifications to ease the process of compilation.

## Compilation Stages

The compilation process is divided into several stages, each responsible for different aspects of translating WLP4 code into MIPS assembly:

### Scanning
The input code is scanned using the Simplified Maximal Munch algorithm, which breaks the text into a series of tokens suitable for parsing.

### Parsing
The SLR(1) parsing method is employed to parse the tokens into a parse tree. This method provides a good balance between parsing power and complexity, suitable for languages like WLP4.

### Context-Sensitive Analysis
Once a parse tree is constructed, it undergoes context-sensitive analysis to annotate types and perform semantic checks, ensuring that the code adheres to language rules beyond syntactic structure.

### Code Generation
The annotated parse tree is then traversed to generate MIPS assembly code. This step translates the high-level constructs of WLP4 into low-level assembly instructions.

### Code Optimization
Code optimization techniques are applied to improve the efficiency and size of the generated MIPS code. These techniques have reduced the code size from approximately 120kB to 80kB. Key optimization strategies include:

- **Constant Folding**: Evaluates and simplifies constant expressions at compile time, reducing runtime computation.
- **Constant Propagation**: Replaces variables with known constant values throughout the code, simplifying expressions and conditions.
- **Register Allocation**: Utilizes unused MIPS registers to store intermediate values and variables, minimizing memory access by reducing the need for load and store operations.

