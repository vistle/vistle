 #!/usr/bin/env bash

 # ------------------------------------------------------------
 #  build_check.sh
 #
 #  Checks the latest GitHub Actions build status.
 #  If the checks pass, it pulls the repo, runs CMake,
 #  builds with make and logs any failures.
 #
 #  ────────────────────────────────────────────────────────
 #  Dependencies:
 #    • curl        (for GitHub API)
 #    • jq          (JSON parser)
 #    • git         (clone / pull)
 #    • cmake       (configure)
 #    • make        (build)
 #  ------------------------------------------------------------

 set -euo pipefail            # fail fast, treat unset vars as error
 IFS=$'\n\t'

 #########################
 ## Configuration
 #########################

 # ----- GitHub details -----
 GITHUB_REPO="vistle/vistle"     # <--- change to your repo
 RECIPIENTS="e@mail.com"
 API_TOKEN="${GITHUB_API_TOKEN:-}"   # optional PAT (needs repo:status)
 BASE_URL="https://api.github.com"

 # ----- Build directories -----
 REPO_DIR="/path/to/repo/$(basename "$GITHUB_REPO")"
 BUILD_DIR="$REPO_DIR/build-linux64-ompi"
 LOG_FILE="/tmp/build_check.log"
 JOBS=12

 #########################
 ## Helper functions
 #########################

 log() { echo "$(date '+%Y-%m-%d %H:%M:%S') $*" >>"$LOG_FILE"; }

 check_github_status() {
     # 1️⃣ Get the latest workflow run for the default branch
     local branch=$(git -C "$REPO_DIR" rev-parse --abbrev-ref HEAD)
     local runs_url="${BASE_URL}/repos/${GITHUB_REPO}/actions/runs?branch=${branch}&per_page=1"
     local resp
     if [[ -n $API_TOKEN ]]; then
         resp=$(curl -sSLH "Authorization: token ${API_TOKEN}" "$runs_url")
     else
         resp=$(curl -sSL "$runs_url")
     fi

     # 2️⃣ Extract the conclusion (success / failure / etc.)
     local conclusion
     conclusion=$(echo "$resp" | jq -r '.workflow_runs[0].conclusion')
     if [[ $conclusion == "null" ]]; then
         log "❌ No completed run found for branch '$branch'."
         return 1
     fi

     if [[ $conclusion != "success" ]]; then
         log "⚠️  Latest run concluded with: $conclusion."
         return 1
     fi

     log "✅ GitHub Actions build succeeded."
     return 0
 }

 configure_and_build() {
     mkdir -p "$BUILD_DIR"
     pushd "$BUILD_DIR"

     # ---- CMake configure ----
     if ! cmake .. >>"$LOG_FILE" 2>&1; then
         log "❌ CMake configuration failed. Check $LOG_FILE for details."
         popd
         return 1
     fi
     log "✅ CMake configuration succeeded."

     # ---- Build with make ----
     if ! make -j$JOBS >>"$LOG_FILE" 2>&1; then
         log "❌ Make build failed. See $LOG_FILE."
         popd
         return 1
     fi
     log "✅ Build succeeded."
     popd
 }

send_email() {
    local subject="$1"
    local body="$2"

    echo "$body" | mailx -s "$subject" $RECIPIENTS
}

 #########################
 ## Main flow
 #########################

 # If the repo dir does not exist, clone it first
 if [[ ! -d "$REPO_DIR/.git" ]]; then
     log "⚙️  Cloning repository $GITHUB_REPO into $REPO_DIR"
     git clone --recursive "https://github.com/${GITHUB_REPO}.git" "$REPO_DIR"
 fi

 # Pull the latest changes
 log "🔄 Pulling latest commits."
 git -C "$REPO_DIR" fetch --all
 git -C "$REPO_DIR" reset --hard origin/$(git -C "$REPO_DIR" rev-parse --abbrev-ref HEAD)

 # Check GitHub build status
 if ! check_github_status; then
     exit 1
 fi

 # If the checks passed, run local build
 if configure_and_build; then
     exit 1
     # send_email "✅ Build succeeded on $(hostname)" \
     #     "The daily build for ${GITHUB_REPO} finished successfully.\n\nSee $LOG_FILE for details."
 fi
 # else
     # send_email "❌ Build FAILED on $(hostname)" \
     #     "Something went wrong during the nightly build of ${GITHUB_REPO}.\nCheck $LOG_FILE for details."
 # fi

 log "🎉 All steps completed successfully."
 exit 0
