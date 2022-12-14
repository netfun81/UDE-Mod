#ifndef _UDE_I18N_H
#define _UDE_I18N_H

#undef _
#undef N_

#ifdef ENABLE_NLS

#include <gettext.h>

#define _(String) gettext (String)

#ifdef gettext_noop
#define N_(String) gettext_noop (String)
#else /* !gettext_noop */
#define N_(String) (String)
#endif /* !gettext_noop */

#else /* !ENABLE_NLS */

#define _(String) (String)
#define N_(String) (String)

#endif /* !ENABLE_NLS */

#endif /* !_UDE_I18N_H */
