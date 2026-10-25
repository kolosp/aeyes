# aeyes

"Circle to Search" for KDE Plasma. Press a shortcut, drag a region with Spectacle, get an
explanation from a vision-capable AI model (Markdown + LaTeX), and keep chatting about it.
Sessions are saved and browseable (`aeyes --history`).

C++20 / Qt 6 / KDE Frameworks 6 / Kirigami. Answers are rendered with QtWebEngine + KaTeX
(bundled, works offline). The app is started on demand and exits when you close the window.

## Privacy

Whatever is in the selected region leaves your machine. **The Gemini free tier may use submitted
content to improve Google's models, and humans may read it**; a billing-enabled key does not.
aeyes shows this on first run and never sends anything before you acknowledge it. You can use a
local model through the OpenAI-compatible provider (e.g. Ollama at `http://localhost:11434/v1`).
API keys are stored in KWallet (or take `GEMINI_API_KEY` / `OPENAI_API_KEY` from the environment).

## Build & install

    cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=~/.local
    cmake --build build && ctest --test-dir build && cmake --install build

Needs: extra-cmake-modules, qt6-base/declarative/webengine, kirigami, ki18n, kconfig,
kcoreaddons, kdbusaddons, kcrash, qtkeychain-qt6, spectacle (runtime); `node` is optional (runs
the Markdown/LaTeX test).

## Shortcut

The desktop file is also installed into `share/kglobalaccel/`, so Plasma lists **aeyes** in
System Settings → Keyboard → Shortcuts (default `Meta+Shift+A` on "Capture and Ask"). Plasma reads that directory at
login, so log out and in once after the first install. A Copilot key typically reports as
`Meta+Shift+F23`; assign it there.

## Usage

    aeyes              home screen (history); never captures
    aeyes --capture    capture a region and ask (bind your shortcut to this)
    aeyes --history    browse / continue past sessions
    aeyes --settings   preferences

`AEYES_GRAB=file.png` saves a screenshot of the window after a delay (for docs/CI).
