# Adding a New Agent to the BITE Zoo

1. Copy this `_template/` directory to `docs/bite/zoo/<your-agent-slug>/`.
2. Replace `agent.js` with your JavaScript BITE agent definition (keep the `.js` file standalone so it stays directly downloadable).
3. Add a thumbnail image (e.g., `thumb.jpg` or `thumb.png`) and reference it in `index.qmd` under `image:`, or remove the `image:` field to use the default placeholder.
4. Update `index.qmd` frontmatter (`title`, `subtitle`, `description`, `categories`, `author`, `date`, `host`, `cell-size`) and prose sections (`Ecological Background`, `Model Implementation`, `References`).
5. Preview locally from `docs/` via `quarto preview` and open a pull request.
