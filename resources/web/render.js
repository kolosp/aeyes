"use strict";
const md = window.markdownit({ html: false, linkify: true, breaks: false });
// Make links safe: always open externally (QML intercepts navigation).
const thread = document.getElementById("thread");
let messages = [];

function esc(s) { return s.replace(/[&<>"]/g, c => ({ "&": "&amp;", "<": "&lt;", ">": "&gt;", '"': "&quot;" }[c])); }

// Math is lifted out before Markdown (which would mangle \, _, * inside formulas)
// and put back as KaTeX HTML afterwards. Code spans/fences are left alone.
function renderMarkdown(src) {
    const maths = [];
    const stash = (tex, display) => { maths.push({ tex, display }); return "AEYESMATH" + (maths.length - 1) + "X"; };
    const parts = src.split(/(```[\s\S]*?(?:```|$)|`[^`\n]*`)/g);
    for (let i = 0; i < parts.length; i += 2) {
        parts[i] = parts[i]
            .replace(/\$\$([\s\S]+?)\$\$/g, (_, t) => stash(t, true))
            .replace(/\\\[([\s\S]+?)\\\]/g, (_, t) => stash(t, true))
            .replace(/\\\(([\s\S]+?)\\\)/g, (_, t) => stash(t, false))
            .replace(/(?<![\\$])\$(?![\s$])([^$\n]+?)(?<![\s\\])\$(?!\d)/g, (_, t) => stash(t, false));
    }
    let html = md.render(parts.join(""));
    return html.replace(/AEYESMATH(\d+)X/g, (_, n) => {
        const m = maths[+n];
        try { return katex.renderToString(m.tex.trim(), { displayMode: m.display, throwOnError: false, output: "html" }); }
        catch (e) { return esc(m.tex); }
    });
}

function renderMessage(m, i) {
    const el = document.createElement("div");
    el.className = "msg " + m.role;
    el.id = "m" + i;
    fill(el, m, i);
    return el;
}

function fill(el, m, i) {
    let h = "";
    if (m.role === "user") {
        if (m.thumbs && m.thumbs.length)
            h += '<div class="thumbs">' + m.thumbs.map(t => '<img src="' + t + '">').join("") + "</div>";
        if (m.text) h += '<div class="bubble">' + renderMarkdown(m.text) + "</div>";
    } else {
        h += '<div class="meta"><span>' + esc(m.model || "") + "</span>";
        if (m.status === "streaming") h += '<span class="spinner"></span>';
        else if (m.text) h += '<a href="aeyes://copy/' + i + '">Copy</a>';
        h += "</div>";
        h += '<div class="body">' + renderMarkdown(m.text || "") + "</div>";
        if (m.error) h += '<div class="error">' + esc(m.error) + "</div>";
    }
    el.innerHTML = h;
}

function nearBottom() { return window.innerHeight + window.scrollY >= document.body.scrollHeight - 80; }
function toBottom() { window.scrollTo(0, document.body.scrollHeight); }

window.aeyesTheme = function (t) {
    const s = document.documentElement.style;
    s.setProperty("--bg", t.bg); s.setProperty("--fg", t.fg); s.setProperty("--muted", t.muted);
    document.documentElement.classList.toggle("dark", !!t.dark);
    s.setProperty("--accent", t.accent); s.setProperty("--border", t.border); s.setProperty("--alt", t.alt);
};

window.aeyesSet = function (list) {
    const stick = nearBottom() || messages.length === 0;
    messages = list;
    thread.replaceChildren(...messages.map(renderMessage));
    if (stick) toBottom();
};

window.aeyesLast = function (text, status, error) {
    if (!messages.length) return;
    const i = messages.length - 1, m = messages[i];
    const stick = nearBottom();
    m.text = text; if (status) m.status = status; m.error = error || m.error;
    fill(document.getElementById("m" + i), m, i);
    if (stick) toBottom();
};
