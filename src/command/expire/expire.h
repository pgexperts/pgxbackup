/***********************************************************************************************************************************
Expire Command

Removes backups and WAL according to the configured retention policy. Backups always come first (so dependents go before the full
they descend from), then WAL ranges no longer needed by any retained backup, then backup history manifests. Backup retention and
archive retention are tracked independently — losing PITR is a permitted policy outcome.
***********************************************************************************************************************************/
#ifndef COMMAND_EXPIRE_EXPIRE_H
#define COMMAND_EXPIRE_EXPIRE_H

/***********************************************************************************************************************************
Functions
***********************************************************************************************************************************/
// Expire backups and archives
FN_EXTERN void cmdExpire(void);

#endif
