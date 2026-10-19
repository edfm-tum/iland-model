# iLand JavaScript API Source Documentation

This directory contains the JavaScript doc files (e.g., mock implementations with JSDoc comments) that define the iLand JavaScript API. 

> [!IMPORTANT]
> The legacy `yuidoc` command-line tool is **deprecated** and no longer used to generate documentation. Instead, a custom Python parser scans these files to build Quarto-based documentation pages directly integrated into the main documentation site.

## Directory Structure

*   `iLand/`: JavaScript files with API definitions/comments for the core iLand classes (e.g., `Globals`, `Tree`, `Grid`, `Map`).
*   `ABE/`: JavaScript files with API definitions/comments for the Agent-Based Forest Management Engine (ABE) classes (e.g., `Agent`, `FMEngine`, `Stand`, `TreeList`).
*   `yuidoc.json`: Legacy configuration file retained for backwards compatibility/reference.

## How the Documentation Pipeline Works

Instead of running YUIDoc, the documentation is parsed and generated using the Python script `docs/apidoc/generate.py`.

### 1. Generating/Updating reference files (`.qmd` files)
When you modify or add documentation comments in any `.js` file in this directory (or in `src/abe-lib`), you must run the python generator script:

```bash
# Run from the repository root:
python3 docs/apidoc/generate.py
```

This script parses all comment blocks in:
- `src/apidoc/iLand/*.js`
- `src/apidoc/ABE/*.js`
- `src/abe-lib/**/*.js`

And generates/updates the markdown reference files in `docs/apidoc/classes/` (split into `iland/`, `abe/`, and `abe-library/`).

### 2. Building the site
After generating the Quarto markdown (`.qmd`) files, you compile the static website from the `docs/` directory:

```bash
# From the docs/ directory:
quarto render
```

---

## Writing Documentation Comments

The custom parser supports standard JSDoc/YUIDoc tags within `/** ... */` comment blocks:

*   `@class <ClassName>`: Declares a new class.
*   `@method <methodName>`: Declares a method. Use `@param {Type} <name> <description>` and `@return {ReturnType} <description>` to detail inputs and outputs.
*   `@property <propertyName>`: Declares a property. Include `@type {Type}` and optionally `@readonly` if applicable.
*   `@example`: Begins a code block example.
*   `{{#crossLink "ClassName"}}{{/crossLink}}`: Creates a link to another class or member. The parser resolves this automatically to the correct relative path across sub-chapters.

Refer to the main pipeline documentation in [docs/apidoc/README.md](../../docs/apidoc/README.md) for more details.
