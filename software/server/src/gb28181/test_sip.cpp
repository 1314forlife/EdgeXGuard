#include <stdio.h>
#include <stdlib.h>
#include <ctime>
#include <sys/time.h>
#include <osip2/osip.h>
#include <eXosip2/eXosip.h>

int main() {
    osip_t *osip = NULL;
    eXosip_t *excontext = NULL;

    // 1. 创建 eXosip 上下文
    excontext = eXosip_malloc();
    if (!excontext) {
        printf("❌ eXosip 内存分配失败\n");
        return 1;
    }

    // 2. 初始化 osip
    int ret = osip_init(&osip);
    if (ret == 0) {
        printf("✅ osip 初始化成功\n");
    } else {
        printf("❌ osip 初始化失败: %d\n", ret);
        free(excontext);
        return 1;
    }

    // 3. 初始化 eXosip (传入上下文)
    ret = eXosip_init(excontext);
    if (ret == 0) {
        printf("✅ eXosip 初始化成功\n");
    } else {
        printf("❌ eXosip 初始化失败: %d\n", ret);
    }

    // 4. 清理
    eXosip_quit(excontext);
    if (osip) {
        osip_release(osip);
    }
    if (excontext) {
        free(excontext);
    }

    printf("✅ 测试完成\n");
    return 0;
}
