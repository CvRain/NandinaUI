#ifndef GLOBAL_EXPORT_HPP
#define GLOBAL_EXPORT_HPP

#include <QtGlobal>

#if defined(NANDINA_LIBRARY)
#define NANDINA_EXPORT Q_DECL_EXPORT
#else
#define NANDINA_EXPORT Q_DECL_IMPORT
#endif

#endif // GLOBAL_EXPORT_HPP
