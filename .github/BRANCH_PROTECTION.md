# Branch Protection For `main`

These settings make the repository safer to evolve and look strong in a thesis or project review.

## Recommended GitHub Settings

Open `Settings` -> `Branches` -> `Add branch protection rule` and target `main`.

Enable:

- `Require a pull request before merging`
- `Require approvals`: `1`
- `Dismiss stale pull request approvals when new commits are pushed`
- `Require status checks to pass before merging`
- `Require branches to be up to date before merging`
- `Require conversation resolution before merging`
- `Do not allow bypassing the above settings`

## Required Status Checks

Mark these CI jobs as required:

- `Repo Hygiene`
- `Python Quality`
- `Node Server`
- `Smart Contracts`
- `Docker Build (agent)`
- `Docker Build (zk-inference)`
- `Docker Build (dfl-worker)`
- `Docker Build (smart-contracts)`
- `Runtime Smoke Test`

## Practical Solo Workflow

Even for a single-author thesis repository, a lightweight PR flow is useful:

1. Create a short-lived branch for a change.
2. Push the branch and open a pull request into `main`.
3. Wait for the CI checks to finish.
4. Merge only when the required checks are green.

This gives you a visible record that the code, containers, and basic runtime path were all checked before landing on `main`.
