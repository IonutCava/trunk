///////////////////////////////////////////////////////////////////////////////
// Written by Philip Rideout in April 2010
// Covered by the MIT License

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "Headers/bstrlib.h"
#include "Headers/glsw.h"

#ifdef WIN32
#pragma warning(disable:4996) // allow "fopen"
#endif

///////////////////////////////////////////////////////////////////////////////
// PRIVATE TYPES

typedef struct glswListRec
{
    bstring Key;
    bstring Value;
    struct glswListRec* Next;
} glswList;

struct glswContextRec
{
    bstring PathPrefix;
    bstring PathSuffix;
    bstring ErrorMessage;
    glswList* TokenMap;
    glswList* ShaderMap;
    glswList* LoadedEffects;
};

///////////////////////////////////////////////////////////////////////////////
// PRIVATE GLOBALS

thread_local glswContext* __glsw__Context = 0;

///////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS

static int __glsw__Alphanumeric(char c)
{
    return
        (c >= 'A' && c <= 'Z') ||
        (c >= 'a' && c <= 'z') ||
        (c >= '0' && c <= '9') ||
        c == '_' || c == '.';
}

static void __glsw__FreeList(glswList* pNode)
{
    while (pNode)
    {
        glswList* pNext = pNode->Next;
        bdestroy(pNode->Key);
        bdestroy(pNode->Value);
        free(pNode);
        pNode = pNext;
    }
}

///////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS

static int glswClear(glswContext* gc)
{
    __glsw__FreeList(gc->ShaderMap);
    __glsw__FreeList(gc->LoadedEffects);
    gc->ShaderMap = NULL;
    gc->LoadedEffects = NULL;
    return 0;
}

glswContext* glswGetCurrentContext()
{
    return __glsw__Context;
}

void glswSetCurrentContext(glswContext* gc) {
    __glsw__Context = gc;
}

void glswClearCurrentContext()
{
    glswClear(glswGetCurrentContext());
}

int glswInit()
{
    if (glswGetCurrentContext())
    {
        bdestroy(glswGetCurrentContext()->ErrorMessage);
        glswGetCurrentContext()->ErrorMessage = bfromcstr("Already initialized.");
        return 0;
    }

    glswSetCurrentContext((glswContext*) calloc(sizeof(glswContext), 1));

    return 1;
}

int glswShutdown()
{
    glswContext* gc = glswGetCurrentContext();

    if (!gc)
    {
        return 0;
    }

    bdestroy(gc->PathPrefix);
    bdestroy(gc->PathSuffix);
    bdestroy(gc->ErrorMessage);
    glswClear(gc);
    __glsw__FreeList(gc->TokenMap);
    gc->TokenMap = NULL;
    free(gc);
    glswSetCurrentContext(0);

    return 1;
}

int glswSetPath(const char* pathPrefix, const char* pathSuffix)
{
    glswContext* gc = glswGetCurrentContext();

    if (!gc)
    {
        return 0;
    }

    if (!gc->PathPrefix) {
        gc->PathPrefix = bfromcstr(pathPrefix);
    } else {
        bstring prefix = bfromcstr(pathPrefix);
        if (bstrcmp(gc->PathPrefix, prefix) != 0) {
            bassign(gc->PathPrefix, prefix);
        }
        else {
            bdestroy(prefix);
        }
    }

    if (!gc->PathSuffix) {
        gc->PathSuffix = bfromcstr(pathSuffix);
    } else {
        bstring suffix = bfromcstr(pathSuffix);
        if (bstrcmp(gc->PathSuffix, suffix) != 0) {
            bassign(gc->PathSuffix, suffix);
        }
        else {
            bdestroy(suffix);
        }
    }

    return 1;
}

const char* glswGetShader(const char* pEffectKey)
{
    glswContext* gc = glswGetCurrentContext();

    bstring effectKey = 0;
    glswList* closestMatch = 0;
    struct bstrList* tokens = 0;
    bstring effectName = 0;
    glswList* pLoadedEffect = 0;
    glswList* pShaderEntry = 0;
    bstring shaderKey = 0;

    if (!gc) {
        return 0;
    }

    // Extract the effect name from the effect key
    effectKey = bfromcstr(pEffectKey);
    tokens = bsplit(effectKey, '.');
    if (!tokens || !tokens->qty)
    {
        bdestroy(gc->ErrorMessage);
        gc->ErrorMessage = bformat("Malformed effect key key '%s'.", pEffectKey);
        bstrListDestroy(tokens);
        bdestroy(effectKey);
        return 0;
    }
    effectName = tokens->entry[0];

    // Check if we already loaded this effect file
    pLoadedEffect = gc->LoadedEffects;
    while (pLoadedEffect) {
        if (biseq(pLoadedEffect->Key, effectName) == 1)  {
            break;
        }
        pLoadedEffect = pLoadedEffect->Next;
    }

    // If we haven't loaded this file yet, load it in
    if (!pLoadedEffect)
    {
        bstring effectContents = 0;
        struct bstrList* lines = 0;
        int lineNo = 0;
        {
            FILE* fp = 0;
            bstring effectFile = 0;

            // Decorate the effect name to form the fullpath
            effectFile = bstrcpy(effectName);
            binsert(effectFile, 0, gc->PathPrefix, '?');
            bconcat(effectFile, gc->PathSuffix);

            // Attempt to open the file
            fp = fopen((const char*) effectFile->data, "rb");
            if (!fp)
            {
                bdestroy(gc->ErrorMessage);
                gc->ErrorMessage = bformat("Unable to open effect file '%s'.", effectFile->data);
                bdestroy(effectFile);
                bdestroy(effectKey);
                bstrListDestroy(tokens);
                return 0;
            }

            // Add a new entry to the front of gc->LoadedEffects
            {
                glswList* temp = gc->LoadedEffects;
                gc->LoadedEffects = (glswList*) calloc(sizeof(glswList), 1);
                gc->LoadedEffects->Key = bstrcpy(effectName);
                gc->LoadedEffects->Next = temp;
            }

            // Read in the effect file
            effectContents = bread_gl((bNread) fread, fp);
            fclose(fp);
            bdestroy(effectFile);
            effectFile = 0;
        }

        lines = bsplit(effectContents, '\n');
        bdestroy(effectContents);
        effectContents = 0;

        for (lineNo = 0; lineNo < lines->qty; lineNo++)
        {
            bstring line = lines->entry[lineNo];

            // If the line starts with "--", then it marks a new section
            if (blength(line) >= 2 && line->data[0] == '-' && line->data[1] == '-')
            {
                // Find the first character in [A-Za-z0-9_].
                int colNo = 0;
                for (colNo = 2; colNo < blength(line); colNo++)
                {
                    char c = line->data[colNo];
                    if (__glsw__Alphanumeric(c))
                    {
                        break;
                    }
                }

                // If there's no alphanumeric character,
                // then this marks the start of a new comment block.
                if (colNo >= blength(line))
                {
                    bdestroy(shaderKey);
                    shaderKey = 0;
                }
                else
                {
                    // Keep reading until a non-alphanumeric character is found.
                    int endCol = 0;
                    for (endCol = colNo; endCol < blength(line); endCol++)
                    {
                        char c = line->data[endCol];
                        if (!__glsw__Alphanumeric(c))
                        {
                            break;
                        }
                    }

                    bdestroy(shaderKey);
                    shaderKey = bmidstr(line, colNo, endCol - colNo);

                    // Add a new entry to the shader map.
                    {
                        glswList* temp = gc->ShaderMap;
                        gc->ShaderMap = (glswList*) calloc(sizeof(glswList), 1);
                        gc->ShaderMap->Key = bstrcpy(shaderKey);
                        gc->ShaderMap->Next = temp;
                        gc->ShaderMap->Value = bformat("//__LINE_OFFSET_\n");

                        binsertch(gc->ShaderMap->Key, 0, 1, '.');
                        binsert(gc->ShaderMap->Key, 0, effectName, '?');
                    }

                    // Check for a version mapping.
                    if (gc->TokenMap)
                    {
                        tokens = bsplit(shaderKey, '.');
                        assert(tokens);

                        glswList* pTokenMapping = gc->TokenMap;

                        while (pTokenMapping)
                        {
                            bstring directive = 0;
                            int tokenIndex = 0;

                            // An empty key in the token mapping means "always prepend this directive".
                            // The effect name itself is also checked against the token mapping.
                            if (blength(pTokenMapping->Key) == 0 ||
                                biseq(pTokenMapping->Key, effectName) == 1)
                            {
                                directive = pTokenMapping->Value;
                                binsert(gc->ShaderMap->Value, 0, directive, '?');
                            }

                            // Check all tokens in the current section divider for a mapped token.
                            for (tokenIndex = 0; tokenIndex < tokens->qty && !directive; tokenIndex++)
                            {
                                bstring token = tokens->entry[tokenIndex];
                                if (biseq(pTokenMapping->Key, token) == 1)
                                {
                                    directive = pTokenMapping->Value;
                                    binsert(gc->ShaderMap->Value, 0, directive, '?');
                                }
                            }

                            pTokenMapping = pTokenMapping->Next;
                        }

                        bstrListDestroy(tokens);
                        tokens = 0;
                    }
                }
                continue;
            }
            if (shaderKey)
            {
                bconcat(gc->ShaderMap->Value, line);
                bconchar(gc->ShaderMap->Value, '\n');
            }
        }

        // Cleanup
        bstrListDestroy(lines);
        lines = 0;
        bdestroy(shaderKey);
        shaderKey = 0;
    }

    // Find the longest matching shader key
    pShaderEntry = gc->ShaderMap;

    while (pShaderEntry)
    {
        if (binstr(effectKey, 0, pShaderEntry->Key) == 0 &&
            (!closestMatch || blength(pShaderEntry->Key) > blength(closestMatch->Key)))
        {
            closestMatch = pShaderEntry;
        }

        pShaderEntry = pShaderEntry->Next;
    }

    bstrListDestroy(tokens);
    tokens = 0;
    bdestroy(effectKey);
    effectKey = 0;

    if (!closestMatch)
    {
        bdestroy(gc->ErrorMessage);
        gc->ErrorMessage = bformat("Could not find shader with key '%s'.", pEffectKey);
        return 0;
    }

    return (const char*) closestMatch->Value->data;
}

const char* glswGetError()
{
    glswContext* gc = glswGetCurrentContext();

    if (!gc)
    {
        return "The glsw API has not been initialized.";
    }

    return (const char*) (gc->ErrorMessage ? gc->ErrorMessage->data : 0);
}

int glswAddDirectiveToken(const char* token, const char* directive)
{
    glswContext* gc = glswGetCurrentContext();
    glswList* temp;

    if (!gc)
    {
        return 0;
    }

    temp = gc->TokenMap;
    gc->TokenMap = (glswList*) calloc(sizeof(glswContext), 1);
    if (!gc->TokenMap) {
        return 0;
    }
    gc->TokenMap->Key = bfromcstr(token);
    gc->TokenMap->Value = bfromcstr(directive);
    gc->TokenMap->Next = temp;

    bconchar(gc->TokenMap->Value, '\n');

    return 1;
}
