# iLand JavaScript API Reference Generator

> [!NOTE]
> For instructions on setup, local preview, and general documentation contributor guidelines, please refer to the main [iLand Documentation Contributor Guide](file:///home/werner/dev/iland-model/docs/README.md).

This directory contains the pipeline to parse, generate, and structure the JavaScript API reference documentation for the iLand project. It replaces the legacy `yuidoc` tool and integrates the API reference directly into the unified Quarto-based documentation site.

## Directory Structure

*   [index.qmd](index.qmd): The main landing page for the JavaScript API. Renders as a card-based dashboard linking to the core sub-chapters.
*   [generate.py](generate.py): The Python parser script that extracts doc comments from source files and generates `.qmd` reference pages.
*   `classes/`: Directory where generated `.qmd` files are written (separated into subfolders):
    *   `classes/iland/`: API documentation for iLand Core classes.
    *   `classes/abe/`: API documentation for Agent-Based Management (ABE) classes.
    *   `classes/abe-library/`: API documentation for the ABE Javascript Library (`abe-lib.js`).

## How to Regenerate the Documentation

To regenerate the documentation after modifying source code comments, run the generator script:

```bash
# Run from the repository root:
python3 docs/apidoc/generate.py

# Or run from this directory:
python3 generate.py
```

After generating the files, re-render the Quarto site to compile the HTML pages:

```bash
# From the docs/ directory:
quarto render
```

## Adding and Updating Documentation Comments

The generator parses comments directly from source files in:
*   `src/apidoc/iLand/*.js`
*   `src/apidoc/ABE/*.js`
*   `src/abe-lib/**/*.js`

Write doc comments in standard JSDoc/YUIDoc format using `/** ... */` comment blocks:

```javascript
/**
 * Description of the class.
 * @class ClassName
 */

/**
 * Description of the method.
 * @method methodName
 * @param {Type} parameterName Description of parameter.
 * @return {ReturnType} Description of return value.
 * @example
 *     // Example usage:
 *     Globals.methodName(args);
 */
```

### Cross-Links

You can link to other classes or methods using YUI-style `{{#crossLink}}` tags. The parsing pipeline will automatically resolve these to the correct relative `.qmd` file path across sub-chapters:

*   Link to a class: `{{#crossLink "Grid"}}{{/crossLink}}`
*   Link to a method: `{{#crossLink "Grid/load:method"}}{{/crossLink}}`
*   Link to a property: `{{#crossLink "Grid/count:property"}}{{/crossLink}}`
