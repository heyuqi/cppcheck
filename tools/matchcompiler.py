#!/usr/bin/python

import re
import glob

def compileCmd(tok):
    if tok == '%any%':
        return 'true'
    elif tok == '%bool%':
        return 'tok->isBoolean()'
    elif tok == '%char%':
        return '(tok->type()==Token::eChar)'
    elif tok == '%comp%':
        return 'tok->isComparisonOp()'
    elif tok == '%num%':
        return 'tok->isNumber()'
    elif tok == '%op%':
        return 'tok->isOp()'
    elif tok == '%or%':
        return '(tok->str()=="|")'
    elif tok == '%oror%':
        return '(tok->str()=="||")'
    elif tok == '%num%':
        return 'tok->isNumber()'
    elif tok == '%str%':
        return '(tok->type()==Token::eString)'
    elif tok == '%type%':
        return '(tok->isName() && tok->varId()==0U && tok->str() != "delete")'
    elif tok == '%var%':
        return 'tok->isName()'
    elif tok == '%varid%':
        return '(tok->isName() && tok->varId()==varid)'
    elif (len(tok)>2) and (tok[0]=="%"):
        print "unhandled:" + tok
    return '(tok->str()=="'+tok+'")'

def compilePattern(pattern, nr, varid):
    arg2 = ''
    if varid:
        arg2 = ', const unsigned int varid'
    ret = '// ' + pattern + '\n'
    ret = ret + 'static bool match' + str(nr) + '(const Token *tok'+arg2+') {\n'
    tokens = pattern.split(' ')
    gotoNextToken = ''
    for tok in tokens:
        if tok == '':
            continue
        ret = ret + gotoNextToken
        gotoNextToken = '    tok = tok->next();\n'

        # [abc]
        if (len(tok) > 2) and (tok[0] == '[') and (tok[-1] == ']'):
            ret = ret + '    if (!tok || tok->str().size()!=1U || !strchr("'+tok[1:-1]+'", tok->str()[0]))\n'
            ret = ret + '        return false;\n'

        # a|b|c
        elif tok.find('|') > 0:
            tokens2 = tok.split('|')
            logicalOp = None
            neg = None
            if "" in tokens2:
                ret = ret + '    if (tok && ('
                logicalOp = ' || '
                neg = ''
            else:
                ret = ret + '    if (!tok || !('
                logicalOp = ' || '
                neg = ''
            first = True
            for tok2 in tokens2:
                if tok2 == '':
                    continue
                if not first:
                    ret = ret + logicalOp
                first = False
                ret = ret + neg + compileCmd(tok2)

            if "" in tokens2:
                ret = ret + '))\n'
                ret = ret + '        tok = tok->next();\n'
                gotoNextToken = ''
            else:
                ret = ret + '))\n'
                ret = ret + '        return false;\n'

        # !!a
        elif tok[0:2]=="!!":
            ret = ret + '    if (tok && tok->str() == "' + tok[2:] + '")\n'
            ret = ret + '        return false;\n'
            gotoNextToken = '    tok = tok ? tok->next() : NULL;\n'

        else:
            ret = ret + '    if (!tok || !' + compileCmd(tok) + ')\n'
            ret = ret + '        return false;\n'
    ret = ret + '    return true;\n}\n'
    return ret

def parseMatch(line, pos1):
    parlevel = 0
    args = []
    argstart = 0
    pos = pos1
    inString = False
    while pos < len(line):
        if inString:
            if line[pos] == '\\':
                pos = pos + 1
            elif line[pos] == '"':
                inString = False
        elif line[pos] == '"':
            inString = True
        elif line[pos] == '(':
            parlevel = parlevel + 1
            if parlevel == 1:
                argstart = pos + 1
        elif line[pos] == ')':
            parlevel = parlevel - 1
            if parlevel == 0:
                ret = []
                ret.append(line[pos1:pos+1])
                for arg in args:
                    ret.append(arg)
                ret.append(line[argstart:pos])
                return ret
        elif line[pos] == ',' and parlevel == 1:
            args.append(line[argstart:pos])
            argstart = pos + 1
        pos = pos + 1

    return None

def convertFile(srcname, destname):
    fin = open(srcname, "rt")
    srclines = fin.readlines()
    fin.close()

    matchfunctions = ''
    matchfunctions = matchfunctions + '#include "token.h"\n'
    matchfunctions = matchfunctions + '#include <string>\n'
    matchfunctions = matchfunctions + '#include <cstring>\n'
    code = ''

    patternNumber = 1
    for line in srclines:
        while True:
            pos1 = line.find('Token::Match(')
            if pos1 == -1:
                pos1 = line.find('Token::simpleMatch(')
            if pos1 == -1:
                break

            res = parseMatch(line, pos1)
            if res == None:
                break
            else:
                assert(len(res)==3 or len(res)==4)  # assert that Token::Match has either 2 or 3 arguments

                g0 = res[0]
                arg1 = res[1]
                arg2 = res[2]
                arg3 = None
                if len(res) == 4:
                    arg3 = res[3]

                res = re.match(r'\s*"([^"]*)"\s*$', arg2)
                if res == None:
                    break  # Non-const pattern - bailout
                else:
                    arg2 = res.group(1)
                    a3 = ''
                    if arg3:
                        a3 = ',' + arg3
                    line = line[:pos1]+'match'+str(patternNumber)+'('+arg1+a3+')'+line[pos1+len(g0):]
                    matchfunctions = matchfunctions + compilePattern(arg2, patternNumber, arg3)
                    patternNumber = patternNumber + 1

        code = code + line

    fout = open(destname, 'wt')
    fout.write(matchfunctions+code)
    fout.close()

# selftests..
def assertEquals(actual,expected):
    if actual!=expected:
        print 'Assertion failed:'
        print actual
        print expected
        assert actual == expected
assertEquals(parseMatch('  Token::Match(tok, ";") ',2), ['Token::Match(tok, ";")','tok',' ";"'])
assertEquals(parseMatch('  Token::Match(tok,', 2), None) # multiline Token::Match is not supported yet
assertEquals(parseMatch('  Token::Match(Token::findsimplematch(tok,")"), ";")', 2), ['Token::Match(Token::findsimplematch(tok,")"), ";")', 'Token::findsimplematch(tok,")")', ' ";"']) # inner function call

# convert all lib/*.cpp files
for f in glob.glob('lib/*.cpp'):
    print f + ' => build/' + f[4:]
    convertFile(f, 'build/'+f[4:])

