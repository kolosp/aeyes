"use strict";
// Apply the host's colours synchronously from ?bg=..&dark=1 so the very first frame is already themed.
(function () {
    const q = new URLSearchParams(location.search), bg = q.get("bg");
    if (bg) document.documentElement.style.setProperty("--bg", bg);
    if (q.get("dark") === "1") document.documentElement.classList.add("dark");
})();
