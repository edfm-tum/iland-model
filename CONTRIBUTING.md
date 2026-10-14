# Contributing to iLand

Welcome to the iLand project! We are happy to receive contributions from the community, whether you are reporting a bug, improving documentation, suggesting new features, or submitting code changes.

---

## Community & Discussions

If you have questions, ideas, or want to discuss planned features before writing code:

* **Discord**: Join our community channel on [Discord](https://discord.gg/e5trneUKav) (or via [tinyurl.com/iland-discord](https://tinyurl.com/iland-discord)). This is the best place to talk with the core developers and other iLand modellers.
* **GitHub Issues**: Use [GitHub Issues](https://github.com/edfm-tum/iland-model/issues) for bug reports and feature requests.

---

## Pull Request Workflow

We use standard GitHub pull requests for code and documentation contributions.

### Important: Target the `dev` Branch
> [!IMPORTANT]
> All pull requests should normally be targeted against the **`dev`** branch (not `main`).  
> The `main` branch is reserved for stable releases. Active development and integration take place on `dev`.

### Step-by-Step Guide

1. **Fork the repository**  
   Click the **Fork** button at the top right of the [iLand repository](https://github.com/edfm-tum/iland-model) to create your own copy on GitHub.

2. **Clone your fork locally**
   ```bash
   git clone https://github.com/<your-username>/iland-model.git
   cd iland-model
   ```

3. **Check out the `dev` branch**
   ```bash
   git checkout dev
   git pull origin dev
   ```

4. **Create a topic/feature branch**  
   Give your branch a descriptive name:
   ```bash
   git checkout -b feature/my-new-feature
   # or
   git checkout -b fix/issue-description
   ```

5. **Make your changes & test**  
   * Build and test iLand locally (e.g. using Qt Creator or the build scripts `./build_iland.sh` / `build_iland.ps1`).
   * Keep commits logical and write clear commit messages.

6. **Push to your fork**
   ```bash
   git push -u origin feature/my-new-feature
   ```

7. **Open a Pull Request**  
   * Go to the [iLand repository on GitHub](https://github.com/edfm-tum/iland-model).
   * Click **Compare & pull request**.
   * **Ensure the base branch is set to `dev`** (base: `dev` $\leftarrow$ compare: `your-branch`).
   * Describe your changes, motivation, and how they were tested.

---

## Documentation Contributions

If you would like to contribute to the official documentation website (guides, wiki, Quarto `.qmd` files, or project settings reference):

* Please see the dedicated **[Documentation Contributor Guide](docs/README.md)**.
* It explains how to set up Quarto, run the local preview server (`quarto preview`), and regenerate outputs and settings references from C++ metadata.

---

## Code Guidelines

* **Coding style**: Follow the existing C++ and Qt conventions used throughout the repository.
* **Project settings**: If you add or modify model settings, make sure to update [`src/iland/res/project_file_metadata.txt`](src/iland/res/project_file_metadata.txt).
* **Scope**: Keep pull requests focused on a single feature or bugfix to facilitate review.
