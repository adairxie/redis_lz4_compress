#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include "redismodule.h"
#include "lz4.h"
#include "lz4frame.h"

#define MB *(1U<<20)

#define COMPRESSIBLE_NOISE_LENGTH (2 MB)

/* Returns non-zero on failure. */
int lz4f_compress(RedisModuleCtx * ctx, const char *input, int inSize, char *output, int outSize)
{
    LZ4F_preferences_t prefs = {{0}};
    prefs.compressionLevel = 0;
    prefs.autoFlush = 0;
    prefs.frameInfo.blockSizeID = 0;
    prefs.frameInfo.blockMode = LZ4F_blockLinked;
    prefs.frameInfo.contentChecksumFlag = LZ4F_noContentChecksum;

    size_t cSize;

    cSize = LZ4F_compressFrame(output, outSize, input, inSize, (LZ4F_preferences_t*)&prefs);
    if (LZ4F_isError(cSize))
    {
        RedisModule_Log(ctx, "debug", "compression failed: %s", LZ4F_getErrorName(cSize));
        return -1;
    }
    RedisModule_Log(ctx, "debug", "Compressed %i bytes into a %i bytes fram \n", (int)inSize, (int)cSize);
    return cSize;
}

int LZ4Compress_RedisCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {
    if (argc != 2) return RedisModule_WrongArity(ctx);

    RedisModuleKey *key = RedisModule_OpenKey(ctx, argv[1], REDISMODULE_READ);
    if (key == NULL)
        return RedisModule_ReplyWithNull(ctx);

    int keytype = RedisModule_KeyType(key);
    if (keytype != REDISMODULE_KEYTYPE_STRING &&
            keytype != REDISMODULE_KEYTYPE_EMPTY)
    {
        RedisModule_CloseKey(key);
        return RedisModule_ReplyWithError(ctx, REDISMODULE_ERRORMSG_WRONGTYPE);
    }
    
    char * s;
    size_t len;

    if (keytype == REDISMODULE_KEYTYPE_STRING) {
        s = RedisModule_StringDMA(key,&len,REDISMODULE_READ);
        RedisModule_Log(ctx, "debug", "key:%s, value: %s, len:%d", argv[1], s, len);
    }

    RedisModule_AutoMemory(ctx); /* Use automatic memory management. */
    int nbytes  = LZ4F_compressFrameBound(COMPRESSIBLE_NOISE_LENGTH, NULL);
    //char *output = RedisModule_PoolAlloc(ctx, nbytes);
    char *output = (char *)malloc(sizeof(char) * nbytes); 
    
    int nwrites  = lz4f_compress(ctx, s, strlen(s), output, nbytes);

    RedisModule_CloseKey(key);
    RedisModule_ReplyWithStringBuffer(ctx, output, nwrites); 
    //RedisModule_ReplyWithSimpleString(ctx, output); 
    RedisModule_ReplicateVerbatim(ctx);

    return REDISMODULE_OK;
}

int RedisModule_OnLoad(RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {
    REDISMODULE_NOT_USED(argv);
    REDISMODULE_NOT_USED(argc);
    if (RedisModule_Init(ctx,"lz4",1,REDISMODULE_APIVER_1)
        == REDISMODULE_ERR) return REDISMODULE_ERR;

    if (RedisModule_CreateCommand(ctx,"lz4.compress",
        LZ4Compress_RedisCommand,"write",0,0,0) == REDISMODULE_ERR)
        return REDISMODULE_ERR;

    return REDISMODULE_OK;
}
