# Contributing to OpenToy

## One active repository

Use **[yury-g/OpenToy-Coursework](https://github.com/yury-g/OpenToy-Coursework)** for all new OpenToy code, CAD, guides, roadmap changes and issues. Its `main` branch is the shared source of truth. The earlier private `yury-g/OpenToy` repository is a historical reference. There is no two-way sync or routine double-publishing.

1. Open a checkout cloned from this repository and check `git remote -v`. Start with a clean working tree, switch to `main`, and run `git pull --ff-only`.
2. Create a short-lived branch for a task, such as `docs/camera-guide` or `feature/tab5-display`. Give each concurrent task its own branch/worktree and each physical device one operator.
3. Make the change, run the checks appropriate to it, and update its project guide and roadmap. Record what was tested, what is installed, and what remains open separately.
4. Push the branch here and open a pull request into `main`. Resolve conflicts on the task branch, rerun affected checks, and merge once reviewed. Start the next task from updated `main`.

The imported feature/device branches and release tags are preserved source snapshots. Start new work from current `main`; consult [the source map](docs/COURSEWORK.md#project-source-map) for current paths. Do not force-push `main`, rewrite historical tags, or merge the old repository's history into this one. If unfinished work exists only in an old checkout, preserve it there, then copy only the reviewed changes into a new branch here and record their origin. Do not repoint an old checkout's remote and push its old history.

Interactive viewers are generated from reviewed assets in this repository. Merging viewer changes into `main` automatically publishes them through GitHub Pages. The READMEs remain the home for the projects and guides; see [Interactive viewers](docs/INTERACTIVE-VIEWERS.md). Keep credentials and machine-specific files outside Git.

Repository publication does not resume parked hardware or authorize a flash. Follow the current owner instructions and each project's device checks.

## Changes and evidence

Start with the [device and project directory](projects/README.md). Open an issue with the board model and revision, app name and version, what you did, and what appeared on the screen. For a visual change, a cropped screen image or renderer preview helps. Label simulations and include the controls needed to reproduce the behavior.

For a pull request, follow the target project’s build and test instructions. Report host checks, target builds and observations from a physical device separately. Identify the source, revision and reuse terms for third-party files. The [hardware gallery](hardware/README.md) shows how existing boards record these details.

Update the [app and device tables](README.md#app-support-matrix) when support changes. New projects can get a home-page card by adding one catalog row and refreshing the README. See the [gallery instructions](docs/PROJECT-ORGANIZATION.md#updating-the-home-gallery).

Keep device identity profiles, provisioning keys, flash backups, personal paths and raw camera captures on your own machine. Use example configurations and review image metadata before sharing. If you discover an exposed key, omit its value from issues, screenshots and patches.
