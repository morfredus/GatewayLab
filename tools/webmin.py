"""webmin.py - minificateurs HTML / CSS / JavaScript SANS dependance, prudents.

Pourquoi ce module existe
-------------------------
Les ressources web sont minifiees a chaque compilation, y compris sur un clone neuf ou
aucune bibliotheque Python optionnelle n'est installee. Un repli naif (supprimer tout ce qui
suit `//`, fusionner toutes les lignes) casserait le code : `"http://..."` serait tronque et une
ligne sans point-virgule se collerait a la suivante.

Ce module lit le source comme le ferait un analyseur lexical (chaines, gabarits `${}`, expressions
regulieres, commentaires) et ne retire que ce qui est sur sans changer le sens :
  - commentaires, indentation, lignes vides ;
  - espaces autour de la ponctuation, QUAND aucun mot ni operateur ne risque de se coller.
Un retour a la ligne est conserve partout ou il peut compter (insertion automatique de `;`).
Au moindre doute (chaine ou gabarit non termine...), le texte est rendu INCHANGE : une page un peu
plus lourde vaut mieux qu'une page cassee.

Si `rjsmin` / `rcssmin` sont installes, ils sont preferes (outils eprouves) ; sinon ce repli suffit.

Usage direct (diagnostic) :
    python tools/webmin.py fichier.js [fichier.css ...]     # affiche le gain
    python tools/webmin.py --out dossier fichier ...         # ecrit les versions minifiees
"""

import re
import sys

try:                                   # outils eprouves, facultatifs
    import rjsmin                      # type: ignore
except ImportError:
    rjsmin = None
try:
    import rcssmin                     # type: ignore
except ImportError:
    rcssmin = None


# ---------------------------------------------------------------------------
# JavaScript
# ---------------------------------------------------------------------------
_WORD = re.compile(r"[A-Za-z0-9_$\u0080-￿]")
_IDENT = re.compile(r"[A-Za-z_$\u0080-￿][A-Za-z0-9_$\u0080-￿]*")
_NUMBER = re.compile(
    r"0[xXbBoO][0-9a-fA-F_]+n?|\d[\d_]*\.?[\d_]*(?:[eE][+-]?\d+)?n?|\.\d[\d_]*(?:[eE][+-]?\d+)?")
# Apres ces mots, un « / » ouvre une expression reguliere (et non une division).
_REGEX_AFTER_WORD = {"return", "typeof", "instanceof", "in", "of", "new", "delete", "void",
                     "throw", "case", "do", "else", "yield", "await"}


class _Unterminated(Exception):
    pass


def _scan_string(s, i):
    q = s[i]
    n = len(s)
    i += 1
    while i < n:
        c = s[i]
        if c == "\\":
            i += 2
            continue
        if c == q:
            return i + 1
        if c == "\n":
            raise _Unterminated("chaine non terminee")
        i += 1
    raise _Unterminated("chaine non terminee")


def _scan_template(s, i):
    n = len(s)
    i += 1
    while i < n:
        c = s[i]
        if c == "\\":
            i += 2
            continue
        if c == "`":
            return i + 1
        if c == "$" and s.startswith("${", i):
            i = _scan_braces(s, i + 2)
            continue
        i += 1
    raise _Unterminated("gabarit non termine")


def _scan_braces(s, i):
    """Parcourt une expression `${ ... }` (accolades equilibrees) et rend l'index apres le `}`."""
    n = len(s)
    depth = 1
    while i < n:
        c = s[i]
        if c in "'\"":
            i = _scan_string(s, i)
            continue
        if c == "`":
            i = _scan_template(s, i)
            continue
        if s.startswith("//", i):
            j = s.find("\n", i)
            i = n if j < 0 else j
            continue
        if s.startswith("/*", i):
            j = s.find("*/", i + 2)
            if j < 0:
                raise _Unterminated("commentaire non termine")
            i = j + 2
            continue
        if c == "{":
            depth += 1
        elif c == "}":
            depth -= 1
            if depth == 0:
                return i + 1
        i += 1
    raise _Unterminated("expression de gabarit non terminee")


def _scan_regex(s, i):
    n = len(s)
    i += 1
    in_class = False
    while i < n:
        c = s[i]
        if c == "\\":
            i += 2
            continue
        if c == "\n":
            raise _Unterminated("expression reguliere non terminee")
        if c == "[":
            in_class = True
        elif c == "]":
            in_class = False
        elif c == "/" and not in_class:
            i += 1
            while i < n and (s[i].isalpha()):
                i += 1
            return i
        i += 1
    raise _Unterminated("expression reguliere non terminee")


def _tokens_js(s):
    """Produit (genre, texte, saut_de_ligne_avant, espace_avant). Genres : word, num, punct, str, tpl, re."""
    i, n = 0, len(s)
    prev_kind, prev_text, prev2_text = None, "", ""
    nl = sp = False
    while i < n:
        c = s[i]
        if c == "\n" or c == " " or c == " ":
            nl = True
            i += 1
            continue
        if c in " \t\r\f\v ﻿":
            sp = True
            i += 1
            continue
        if s.startswith("//", i):
            j = s.find("\n", i)
            i = n if j < 0 else j
            sp = True
            continue
        if s.startswith("/*", i):
            j = s.find("*/", i + 2)
            if j < 0:
                raise _Unterminated("commentaire non termine")
            if "\n" in s[i:j]:
                nl = True
            else:
                sp = True
            i = j + 2
            continue

        if c in "'\"":
            j = _scan_string(s, i)
            kind, text = "str", s[i:j]
        elif c == "`":
            j = _scan_template(s, i)
            kind, text = "tpl", s[i:j]
        elif c == "/":
            division = True
            if prev_kind is None:
                division = False
            elif prev_kind == "word":
                division = prev_text not in _REGEX_AFTER_WORD
            elif prev_kind == "punct":
                if prev_text in (")", "]", "}"):
                    division = True
                elif prev_text in ("+", "-") and prev2_text == prev_text:
                    division = True              # a++ / 2
                else:
                    division = False
            if division:
                j = i + 1
                kind, text = "punct", "/"
            else:
                j = _scan_regex(s, i)
                kind, text = "re", s[i:j]
        elif c.isdigit() or (c == "." and i + 1 < n and s[i + 1].isdigit()):
            m = _NUMBER.match(s, i)
            j = m.end()
            kind, text = "num", s[i:j]
        else:
            m = _IDENT.match(s, i)
            if m:
                j = m.end()
                kind, text = "word", s[i:j]
            else:
                j = i + 1
                kind, text = "punct", c
        yield kind, text, nl, sp
        prev2_text, prev_kind, prev_text = prev_text, kind, text
        nl = sp = False
        i = j


def _needs_space(last_kind, last, kind, text):
    a, b = last[-1], text[0]
    if _WORD.match(a) and _WORD.match(b):
        return True
    if a == b and a in "+-":
        return True                              # a + +b  /  a - -b
    if a == "/" and b in "/*":
        return True                              # ne pas fabriquer un commentaire
    if last_kind == "num" and b == ".":
        return True                              # 1 .toString()
    if last_kind == "re" and _WORD.match(b):
        return True                              # /re/ in x : « in » ne doit pas devenir un drapeau
    return False


def _minify_js_strict(src):
    out = []
    last_kind, last = None, ""
    for kind, text, nl, sp in _tokens_js(src):
        if last_kind is not None:
            if nl:
                # Le retour a la ligne n'est retire que la ou une instruction ne peut pas finir
                # (apres { ( [ , ;) ou ne peut pas commencer (avant } ) ] , ;) : sinon il compte (ASI).
                if last in ("{", "(", "[", ",", ";") or (kind == "punct" and text in "}),];" and len(text) == 1):
                    sep = " " if (_needs_space(last_kind, last, kind, text)) else ""
                else:
                    sep = "\n"
            elif sp and _needs_space(last_kind, last, kind, text):
                sep = " "
            else:
                sep = ""
            out.append(sep)
        out.append(text)
        last_kind, last = kind, text
    return "".join(out)


def minify_js(src):
    """Minifie du JavaScript. Rend le texte INCHANGE si le source n'a pas pu etre lu surement."""
    if rjsmin is not None:
        try:
            return str(rjsmin.jsmin(src)).strip()
        except Exception:
            pass
    try:
        return _minify_js_strict(src).strip()
    except (_Unterminated, IndexError):
        return src


# ---------------------------------------------------------------------------
# CSS
# ---------------------------------------------------------------------------
_CSS_SEGMENT = re.compile(
    r"""(/\*.*?\*/)                      # commentaire : supprime
      | ("(?:\\.|[^"\\\n])*" | '(?:\\.|[^'\\\n])*')   # chaine : conservee
      | (url\(\s*[^)'"\s][^)]*\))      # url(...) non quote : conserve tel quel
    """, re.S | re.X)


def minify_css(css):
    if rcssmin is not None:
        try:
            return str(rcssmin.cssmin(css)).strip()
        except Exception:
            pass
    pieces = []
    pos = 0
    for m in _CSS_SEGMENT.finditer(css):
        pieces.append(("t", css[pos:m.start()]))
        if m.group(1) is None:                    # chaine ou url : conservee
            pieces.append(("k", m.group(0)))
        pos = m.end()
    pieces.append(("t", css[pos:]))

    out = []
    for kind, text in pieces:
        if kind == "k":
            out.append(text)
            continue
        text = re.sub(r"\s+", " ", text)
        text = re.sub(r" ?([{};,>]) ?", r"\1", text)      # espaces autour de { } ; , >
        text = re.sub(r":\s+", ":", text)                 # « color: red » (jamais avant « : » : « a :hover » compte)
        out.append(text)
    res = "".join(out).replace(";}", "}")
    return res.strip()


# ---------------------------------------------------------------------------
# HTML
# ---------------------------------------------------------------------------
_HTML_BLOCK = re.compile(
    r"(?is)(<script\b[^>]*>.*?</script\s*>|<style\b[^>]*>.*?</style\s*>|<pre\b.*?</pre\s*>|<textarea\b.*?</textarea\s*>)")
_JS_TYPES = ("", "text/javascript", "application/javascript", "module")


def _minify_block(block):
    m = re.match(r"(?is)(<(script|style)\b([^>]*)>)(.*?)(</\2\s*>)$", block)
    if not m:
        return block                                  # <pre> / <textarea> : intact
    open_tag, name, attrs, body, close_tag = m.groups()
    if name.lower() == "style":
        return open_tag + minify_css(body) + close_tag
    kind = re.search(r'''type\s*=\s*["']?([^"'\s>]*)''', attrs)
    if re.search(r"\bsrc\s*=", attrs) or (kind and kind.group(1).lower() not in _JS_TYPES):
        return block                                  # script externe ou donnees (JSON, template...)
    return open_tag + minify_js(body) + close_tag


def minify_html(html):
    parts = _HTML_BLOCK.split(html)
    out = []
    for idx, part in enumerate(parts):
        if idx % 2 == 1:                              # bloc script / style / pre / textarea
            out.append(_minify_block(part))
            continue
        part = re.sub(r"<!--(?!\[if|<!).*?-->", "", part, flags=re.S)
        part = re.sub(r">[ \t]*\r?\n\s*<", "><", part)   # indentation entre deux balises
        part = re.sub(r"\s+", " ", part)
        out.append(part)
    return "".join(out).strip()


# ---------------------------------------------------------------------------
# Ligne de commande (diagnostic)
# ---------------------------------------------------------------------------
def minify_by_extension(name, text):
    low = name.lower()
    if low.endswith((".html", ".htm")):
        return minify_html(text)
    if low.endswith(".js"):
        return minify_js(text)
    if low.endswith(".css"):
        return minify_css(text)
    return text


if __name__ == "__main__":
    import os
    args = sys.argv[1:]
    outdir = None
    if args[:1] == ["--out"]:
        outdir, args = args[1], args[2:]
        os.makedirs(outdir, exist_ok=True)
    total_in = total_out = 0
    for path in args:
        with open(path, encoding="utf-8") as handle:
            raw = handle.read()
        mini = minify_by_extension(path, raw)
        total_in += len(raw.encode("utf-8"))
        total_out += len(mini.encode("utf-8"))
        print(f"{path}: {len(raw.encode('utf-8')):>7} -> {len(mini.encode('utf-8')):>7} octets")
        if outdir:
            with open(os.path.join(outdir, os.path.basename(path)), "w", encoding="utf-8", newline="\n") as handle:
                handle.write(mini)
    if total_in:
        print(f"total: {total_in} -> {total_out} octets ({100 * (1 - total_out / total_in):.1f} % de gain)")
