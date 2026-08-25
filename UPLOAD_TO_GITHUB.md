# Upload this project to GitHub

## Option A — easiest: GitHub web interface

1. Sign in to GitHub.
2. Create a new **private** repository named `GammaEngine`.
3. Do not initialise it with a README, `.gitignore`, or licence.
4. Extract the ZIP you received from ChatGPT.
5. In the new GitHub repository, choose **Add file → Upload files**.
6. Drag the contents of the extracted `GammaEngine-GitHub` folder into the upload area.
7. Commit directly to `main`.

After upload:

1. Open **Code → Codespaces**.
2. Choose **Create codespace on main**.
3. The dev container will install CMake/Linux JUCE dependencies and configure the project.

To check the native builds:

1. Open **Actions**.
2. Open the latest **Build Gamma Engine** run.
3. When successful, download:
   - `GammaEngine-macOS`
   - `GammaEngine-Windows`

## Important

Codespaces itself is Linux. Use it for browser-based development and compile checks.

Use GitHub Actions for the native macOS/Windows builds.

The current PianoEngine is included, but no copyrighted/third-party WAV file is bundled.
