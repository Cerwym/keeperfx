#!/usr/bin/env bash
# Sets up GitHub branch protection rules for KeeperFX GitFlow branches.
# Requires: gh CLI authenticated with repo admin access.
# Safe to re-run (idempotent).
#
# Usage: ./scripts/setup-branch-protection.sh [owner/repo]
# Example: ./scripts/setup-branch-protection.sh dkfans/keeperfx

set -euo pipefail

REPO="${1:-$(gh repo view --json nameWithOwner -q .nameWithOwner)}"

if [[ -z "$REPO" ]]; then
  echo "ERROR: Could not determine repository. Pass owner/repo as argument." >&2
  exit 1
fi

echo "Configuring branch protection for: ${REPO}"

# Required status checks — must match the job 'name:' field in the workflow YAML exactly.
REQUIRED_CHECKS='[
  {"context": "Build Windows x86"},
  {"context": "Build Linux x64"},
  {"context": "Build PS Vita"}
]'

protect_branch() {
  local branch="$1"
  echo "  Protecting branch: ${branch}"

  gh api \
    --method PUT \
    -H "Accept: application/vnd.github+json" \
    "repos/${REPO}/branches/${branch}/protection" \
    --input - <<EOF
{
  "required_status_checks": {
    "strict": false,
    "checks": ${REQUIRED_CHECKS}
  },
  "enforce_admins": false,
  "required_pull_request_reviews": null,
  "restrictions": null,
  "allow_force_pushes": false,
  "allow_deletions": false
}
EOF

  echo "  ✓ ${branch} protected"
}

# Protect GitFlow stable branches
for BRANCH in dev master; do
  if gh api "repos/${REPO}/branches/${BRANCH}" --silent 2>/dev/null; then
    protect_branch "$BRANCH"
  else
    echo "  Skipping ${BRANCH} (branch does not exist yet)"
  fi
done

echo ""
echo "Branch protection configured for ${REPO}."
echo "Merge into dev/master now requires:"
echo "  - Build Windows x86 (ci-build.yml)"
echo "  - Build Linux x64   (ci-build.yml)"
echo "  - Build PS Vita     (ci-homebrew.yml)"
