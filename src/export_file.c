#include "export_file.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "svc_kernel/database/database.h"

typedef struct {
    int _fd;
    bool _isHeaderPrinted;
} cmd_export_onefile_t;

int i_cmdExportCallback(void* p_param, sqlite3_stmt* stmt) {
    int i;
    cmd_export_onefile_t *p_fileinfo = (cmd_export_onefile_t*)p_param;
    char buffer[512] = {0};
    if(!p_fileinfo->_isHeaderPrinted) {
        for(i = 0;i < sqlite3_column_count(stmt);i++) {
            if(i > 0)
               strcat(buffer, "|");
            strcat(buffer, (const char*)sqlite3_column_name(stmt, i));
        }
        strcat(buffer, "\n");
        write(p_fileinfo->_fd, buffer, strlen(buffer));
        memset(buffer, 0, sizeof(buffer));
        p_fileinfo->_isHeaderPrinted = true;
    }
    for(i = 0;i < sqlite3_column_count(stmt);i++) {
        if(i > 0)
            strcat(buffer, "|");
        strcat(buffer, (const char*)sqlite3_column_text(stmt, i));
    }
    strcat(buffer, "\n");
    write(p_fileinfo->_fd, buffer, strlen(buffer));
    return 0;
}

int cmdExportFileExec(struct timeval ts, void* pdata, size_t dataSize) {
    if(dataSize != sizeof(cmd_export_cfg_t))
        return -1;
    cmd_export_cfg_t* pcfg = (cmd_export_cfg_t*)pdata;
    cmd_export_onefile_t exportfile;
    SYSLOG(LOG_INFO, "[CMDMGR][EXPORT_FILE](%s)", pcfg->_file_name);
    exportfile._fd = open(pcfg->_file_name, O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR);
    exportfile._isHeaderPrinted = false;
    if(exportfile._fd == -1) {
        SYSLOG(LOG_ERR, "[CMDMGR][EXPORT_FILE](%s) error during opening file", pcfg->_file_name);
        return -1;
    }
    dbExecQuery(getNiuchaczPcapDB(), "select * from packets;", 0, i_cmdExportCallback, &exportfile);
    close(exportfile._fd);
    return 0;
}

int cmdExportFileCreate(void) {
    return 0;
}

int cmdExportFileDestroy(void) {
    return 0;
}
