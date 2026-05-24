# iLand Documentation Contributor Guide

This directory contains the source files for the official iLand documentation website. The site is built using **Quarto**, a modern open-source scientific and technical publishing system.

---

## 1. Setup and Local Preview

### Prerequisites
Make sure you have Quarto installed. You can download it from [quarto.org](https://quarto.org) or use the version bundled with RStudio.

### Previewing the Site Locally
To run a local live-reload preview server while editing `.qmd` files:
```bash
# From within the docs/ directory:
quarto preview
```
This will spin up a local server (typically at `http://localhost:4739/`) that automatically refreshes your browser when you save any document.

### Rendering the Site
To compile the entire site into static HTML assets:
```bash
# Renders the website to the `docs/_site/` folder
quarto render
```

---

## 2. Structure of the Source Files

*   **`_quarto.yml`**: The main configuration file containing metadata, navigation bar settings, sidebar outline structure, and theme customizations.
*   **`index.qmd`**: The main hub page.
*   **`wiki/`**: Directory containing markdown files (`.qmd`) for the model description, demographic processes, and general usage instructions.
*   **`blog/`**: Directory containing news posts and articles.
*   **`apidoc/`**: Contains scripting API descriptions.
*   **`img/`**: Stores images and assets used throughout the documentation.
*   **`custom.css`**: Global CSS styling adjustments (light/dark general layout rules).
*   **`custom-light.scss`**: Sass variables (like primary brand colors) for the light theme profile.
*   **`custom-dark.scss`**: Sass variables and CSS rules tailored specifically for the dark theme profile.

---

## 3. Generating C++ Output Descriptions (`outputs.qmd`)

The details of iLand outputs (database tables, columns, and data types) are maintained directly within the C++ source code of the model. 

To update the outputs page ([outputs.qmd](file:///home/werner/dev/iland-model/docs/wiki/outputs.qmd)) with changes made in the code:

1.  Ensure you are running a build of iLand compiled from the full repository source.
2.  Open the iLand GUI application.
3.  Trigger the output description compilation from the application menu:
    *   Go to **Help** / **Outputs** (or trigger the slot `MainWindow::on_actionOutput_table_description_triggered()`).
4.  The application will automatically detect that it is running inside the repository tree and rewrite the contents of `docs/wiki/outputs.qmd` between the marker comments:
    ```html
    <!-- GENERATED-CODE-START -->
    ... (automatically generated table structures from C++ output definitions) ...
    <!-- GENERATED-CODE-END -->
    ```
5.  After the execution completes, review the diff in `outputs.qmd` and run `quarto render` to compile the changes to the static site.

---

## 4. Managing Documentation Versions

We support multi-version documentation hosting to allow users to view current, historical, or development-level features.

### Deployment Directory Structure on the Web Server:
All versions are hosted under separate subfolders on the server:
```text
/var/www/iland-docs/
├── latest/ -> 2.1/            # A symlink pointing to the current stable folder (e.g. /2.1/)
├── dev/                       # Bleeding-edge build from the master/main branch
├── 2.1/                       # Frozen archive of version 2.1 documentation
└── 2.0/                       # Frozen archive of version 2.0 documentation
```

### The Version Switcher Dropdown
The version selector dropdown is defined in [\_quarto.yml](file:///home/werner/dev/iland-model/docs/_quarto.yml) under `website.navbar.right`. 
To ensure a seamless user experience, we load the [version-switcher.html](file:///home/werner/dev/iland-model/docs/version-switcher.html) script in the page footer. This script intercepts clicks on the dropdown menu and dynamically translates the URL path (e.g. `/dev/wiki/growth.html` to `/2.0/wiki/growth.html`), keeping the user on the page they were reading instead of kicking them back to the homepage.

### How to Release/Freeze a New Documentation Version (e.g., v2.2):
1.  **Branch off release:** Create a git branch `v2.2` from the current development state (`master` / `main`).
2.  **Update dropdown configurations:**
    *   On the new release branch, modify `_quarto.yml` version list to mark it as stable.
    *   On the development branch, add the new version to the list of versions in the dropdown.
3.  **Compile:** Render the documentation on the release branch:
    ```bash
    quarto render
    ```
4.  **Upload:** Deploy the resulting `_site` folder to the `/2.2/` subdirectory on the web server.
5.  **Redirect Stable:** Update the `latest` symlink on the web server to point to `/2.2/`.
