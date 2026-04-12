# VM Translator Error Reference

## 1. Parsing Errors (`PRS`)
Errors encountered during the initial lexical analysis of the input stream.

| Code | Expectation | Actual / Received |
| :--- | :--- | :--- |
| **ERR_PRS_0** | Valid input line | `NULL` pointer |
| **ERR_PRS_1** | Valid output pointer | `NULL` pointer |
| **ERR_PRS_2** | ASCII encoded input | Non-ASCII characters |
| **ERR_PRS_3** | 1 to 3 words per line | Word count > 3 |
| **ERR_PRS_4** | Integer as the 3rd word | `<value>` |
| **ERR_PRS_5** | Integer within valid range | `<value>` |

---

## 2. CLI & Argument Errors (`CLI`)
Errors related to command-line invocation and path validation.

| Code | Expectation | Actual / Received |
| :--- | :--- | :--- |
| **ERR_CLI_0** | 1 or 2 user arguments | `<count>` arguments |
| **ERR_CLI_1** | Accessible input path | `<path>` (`<system error>`) |
| **ERR_CLI_2** | Regular `.vm` file or directory | Invalid `<path>` |
| **ERR_CLI_3** | File ending in `.vm` | `<path>` |
| **ERR_CLI_4** | Path length < `PATH_MAX` | `<path>` |
| **ERR_CLI_5** | Resolvable folder path | `<path>` (`<system error>`) |

---

## 3. Input / Output Errors (`IO`)
Errors occurring during file system interactions and stream management.

| Code | Expectation | Actual / Context |
| :--- | :--- | :--- |
| **ERR_IO_0** | Readable directory | `<path>` (`<system error>`) |
| **ERR_IO_1** | Writable output file | `<path>` (`<system error>`) |
| **ERR_IO_2** | Readable input file | `<path>` (`<system error>`) |
| **ERR_IO_3** | Line length within `VM_LINE_MAX` | `<path>:<line>` |
| **ERR_IO_4** | Valid input file stream | `<path>` (`<system error>`) |
| **ERR_IO_5** | Successfully closed file stream | `<path>` (`<system error>`) |

---

## 4. Translation & Logic Errors (`TRN`)
Errors occurring during the mapping of VM commands to assembly code.

| Code | Expectation | Actual / Context |
| :--- | :--- | :--- |
| **ERR_TRN_0** | Sufficient system memory | Out of Memory (OOM) |
| **ERR_TRN_1** | Valid VM file context | `<path>` |
| **ERR_TRN_2** | Known VM command | `<path>:<line> <command>` |
| **ERR_TRN_3** | Correct word count for specific command | `<path>:<line> <details>` |
| **ERR_TRN_4** | Command/Arg within writer limits | `<path>:<line>` |
| **ERR_TRN_5** | Translatable VM command | `<path>:<line> <command>` |
| **ERR_TRN_6** | Initialized code writer | Initialization failure |
| **ERR_TRN_7** | Successful bootstrap code emission | Write failure |
| **ERR_TRN_8** | Directory containing $\ge 1$ `.vm` file | `<path>` (Empty or no .vm) |
| **ERR_TRN_9** | Emission of shared VM helper routines | Write failure |
| **ERR_TRN_10**| Non-negative index or valid constant (-1) | `<path>:<line> <command>` |