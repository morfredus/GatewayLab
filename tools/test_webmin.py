"""Auto-test de webmin.py : chaque extrait JavaScript est EXECUTE avant et apres minification
(Node.js) et les deux sorties doivent etre identiques. Les cas sont ceux qui cassent les
minificateurs naifs : `//` dans une chaine, retour a la ligne apres `return`, `a\\n++b`,
expressions regulieres, gabarits imbriques, `1 + +2`...

    python tools/test_webmin.py        # code retour 0 si tout passe (JS ignore sans Node)
"""

import os
import shutil
import subprocess
import sys
import tempfile

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import webmin  # noqa: E402

# Pour tester LE REPLI (celui d'un clone neuf), pas rjsmin/rcssmin.
webmin.rjsmin = None
webmin.rcssmin = None

JS_CASES = {
    "chaine avec //": 'console.log("http://x.y/*pas un commentaire*/" + \'a//b\')',
    "ASI sans point-virgule": "var a = 1\nvar b = 2\nconsole.log(a + b)",
    "return puis ligne": "function f(){ return\n 5 }\nconsole.log(f())",
    "ligne ++b": "var a = 1, b = 1\na\n++b\nconsole.log(a, b)",
    "regex et division": 'console.log("a/b".split(/\\//).length, /[/]x/.test("/x"))\nvar x = 8, y = 2, z = 1\nconsole.log(x / y / z)',
    "division apres ++": "var a = 5\nvar r = a++ / 2\nconsole.log(r, a)",
    "gabarits imbriques": "console.log(`a${1 + 1}//b ${ `n${'}'}` }`)",
    "unaires": 'console.log(1 + +"2", 5 - -3, 1 - - 1)\nvar b = 1\nconsole.log(1 + ++b)',
    "commentaires": "/* c */ console.log(1) // fin\n/* multi\nligne */ console.log(2)",
    "mots cles": "console.log(typeof 1, 'x' in {x: 1}, [] instanceof Array, new Date(0).getTime())",
    "fleche et objet": "const f = (a, b) => a * b\nconst o = {a: 1,\n b: 2}\nconsole.log(f(2, 3), o.b)",
    "IIFE": ";(function () { console.log('iife') })()",
    "else catch": "try { throw 1 }\ncatch (e) { console.log(e) }\nif (0) console.log(1)\nelse console.log(2)",
    "nombres": "console.log(1..toString(), 1.5.toFixed(1), (2).toString(), 0xff, 1e3)",
    "continuation de chaine": "console.log('a\\\nb')",
    "division sur ligne suivante": "var a = 4, b = 2, c = 1\nvar r = a\n/ b / c\nconsole.log(r)",
    "regex apres return": "function g(s){ return /a+/.test(s) }\nconsole.log(g('caat'), g('x'))",
    "accolade puis regex-like": "var o = {}\nvar q = 6 / 3\nconsole.log(q)",
}

failures = 0


def check(ok, name):
    global failures
    print(("ok  : " if ok else "FAIL: ") + name)
    if not ok:
        failures += 1


def run_node(code):
    with tempfile.NamedTemporaryFile("w", suffix=".js", delete=False, encoding="utf-8") as handle:
        handle.write(code)
        path = handle.name
    try:
        done = subprocess.run(["node", path], capture_output=True, text=True, timeout=20)
        return done.returncode, done.stdout, done.stderr
    finally:
        os.unlink(path)


if shutil.which("node"):
    for name, src in JS_CASES.items():
        mini = webmin.minify_js(src)
        a, b = run_node(src), run_node(mini)
        check(a[0] == 0 and a == b, f"JS identique apres minification : {name}")
        if a != b:
            print("    original :", a, "\n    minifie  :", b, "\n    texte    :", repr(mini))
    # Source non analysable : rendu inchange plutot que casse.
    broken = "var s = 'pas termine\nconsole.log(s)"
    check(webmin.minify_js(broken) == broken, "JS non analysable : rendu inchange")
    gain = webmin.minify_js(JS_CASES["fleche et objet"])
    check(len(gain) < len(JS_CASES["fleche et objet"]), "JS : le texte est bien plus court")
else:
    print("Node.js absent : tests d'execution JavaScript ignores")

# CSS : structure (le CSS ne s'execute pas).
check(webmin.minify_css("a :hover { color: red; }") == "a :hover{color:red}", "CSS : « a :hover » garde son espace")
check(webmin.minify_css('a { content: "x; y"; }') == 'a{content:"x; y"}', "CSS : chaine protegee")
check(webmin.minify_css("a { background: url(data:image/png;base64,AA==) ; }") == "a{background:url(data:image/png;base64,AA==)}",
      "CSS : url() non quote protege")
check(webmin.minify_css("a > b , c { margin: calc(1px + 2px); }") == "a>b,c{margin:calc(1px + 2px)}", "CSS : calc() garde ses espaces")
check(webmin.minify_css("/* c */ @media (min-width: 600px) and (max-width: 900px) { a { b: c } }")
      == "@media (min-width:600px) and (max-width:900px){a{b:c}}", "CSS : media query")

# HTML : texte en ligne, <pre> et scripts.
html = "<html>\n  <body>\n    <p>un <b>mot</b> <i>deux</i></p>\n    <pre>  a\n   b </pre>\n    <script>\n      var s = 'http://x';  // c\n    </script>\n  </body>\n</html>"
mini = webmin.minify_html(html)
check("<b>mot</b> <i>deux</i>" in mini, "HTML : l'espace entre deux balises en ligne est conserve")
check("<pre>  a\n   b </pre>" in mini, "HTML : <pre> intact")
check("var s='http://x';" in mini and "// c" not in mini, "HTML : script minifie, chaine protegee")
check("\n    " not in mini.replace("<pre>  a\n   b </pre>", ""), "HTML : indentation retiree")

print("Tout est conforme." if not failures else f"ECHEC ({failures})")
sys.exit(1 if failures else 0)
