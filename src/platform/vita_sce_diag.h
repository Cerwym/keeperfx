#ifndef VITA_SCE_DIAG_H
#define VITA_SCE_DIAG_H
/* taiHEN SCE subsystem diagnostic hooks — vita-debug builds only.
 * All symbols are no-ops when VITA_SCE_DIAG is not defined.
 */
#ifdef VITA_SCE_DIAG
#ifdef __cplusplus
extern "C" {
#endif
void vita_diag_install_hooks(void);
void vita_diag_unhook_all(void);
#ifdef __cplusplus
}
#endif
#else
#define vita_diag_install_hooks() ((void)0)
#define vita_diag_unhook_all()    ((void)0)
#endif /* VITA_SCE_DIAG */
#endif /* VITA_SCE_DIAG_H */
