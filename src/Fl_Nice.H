//================================================
// pour des infos voir X_File_Chooser.cxx
// 
// Fl_Nice.* by Asher256 
// Email: asher256@gmail.com
//================================================

#if !defined(LINUX) && !defined(WIN32)
#error "vous devez définir soit LINUX soit WIN32..."
#endif

#if defined(LINUX) && defined(WIN32)
#error "Vous n'avez pas le droit de definir au meme temps LINUX et WIN32"
#endif

#if defined(WIN32) && defined(USE_FLU)
#error "Vous n'avez pas le droit de definir au meme temps WIN32 et USE_FLU"
#endif

//====> les includes
#include <FL/Fl_Window.H>

// Pattern
typedef struct { //le dernier pattern doit être zero
	char *name;   // Par exemple: "Fichier exécutable"
	char *ext;    // par exemple: "exe"
} FN_PATTERN;

// Dir Search
const char *fn_dir_chooser(const char *message, const char *fname);

// File Load/Save 
const char *fn_file_chooser(const char *message, FN_PATTERN *pattern, const char *fname);
const char *fn_save_chooser(const char *message, FN_PATTERN *pattern, const char *fname);

// Messages box
void fn_alert(const char *format, ...);
void fn_message(const char *format, ...);
int fn_ask(const char *format, ...);


