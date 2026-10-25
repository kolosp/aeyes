// Runs the web view's Markdown + LaTeX pipeline in node with a minimal DOM stub.
const fs = require("fs"), path = require("path"), vm = require("vm");
const web = path.join(__dirname, "..", "resources", "web");
const thread = { replaceChildren() {} };
const ctx = { console, document: { getElementById: () => thread, documentElement: { style: { setProperty() {} } } }, innerHeight: 0, scrollY: 0, scrollTo() {} };
ctx.window = ctx; ctx.self = ctx; ctx.globalThis = ctx;
vm.createContext(ctx);
for (const f of ["markdown-it.min.js", "katex/katex.min.js"])
    vm.runInContext(fs.readFileSync(path.join(web, f), "utf8"), ctx, { filename: f });
vm.runInContext(fs.readFileSync(path.join(web, "render.js"), "utf8") + "\nthis.renderMarkdown = renderMarkdown;", ctx);
const r = ctx.renderMarkdown;
let failed = 0;
function check(name, cond, html) { if (!cond) { failed++; console.error("FAIL", name, "\n", html); } }

let h = r("Inline $a_1 * b_2$ and $$\\frac{1}{2}$$ done");
check("inline+display math", (h.match(/class="katex"/g) || []).length === 2 && h.includes("katex-display"), h);
check("underscores in math not italicised", !h.includes("<em>"), h);
h = r("costs $5 and $10 total");
check("currency untouched", !h.includes("katex") && h.includes("$5 and $10"), h);
h = r("\\(x^2\\) and \\[y^2\\]");
check("paren/bracket delimiters", (h.match(/class="katex"/g) || []).length === 2, h);
h = r("```\n$not math$\n```\n`$nor this$`");
check("code untouched", !h.includes("katex") && h.includes("$not math$") && h.includes("$nor this$"), h);
h = r("<script>alert(1)</script>");
check("raw html escaped", !h.includes("<script>"), h);
h = r("$\\badmacro{$");
check("bad tex does not throw", typeof h === "string", h);
h = r("| a | b |\n|---|---|\n| 1 | 2 |");
check("tables", h.includes("<table>"), h);
h = r("unfinished $x^2 + ");
check("streaming partial math is literal", !h.includes("katex-error") && h.includes("$x^2"), h);
process.exit(failed ? 1 : 0);
