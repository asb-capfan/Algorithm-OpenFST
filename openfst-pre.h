#include <stdlib.h>
#include <exception>
#include <string>
// All right, Mr. Google monkey.  Calling exit(1) in a library a bad
// thing, m'kay?

class fst_exception : public std::exception
{
    const std::string msg;
public:
    fst_exception(const std::string& _msg) throw ()
	: msg(_msg) { }
    ~fst_exception() throw ()
	{ }
    const char * what () const throw ()
	{ return msg.c_str(); }
};

#define exit(n) (throw fst_exception((n) ? "ugh" : "ok"))
#include "fst/lib/compat.h"
#undef exit
#include "fst/lib/arcsort.h"
#include "fst/lib/closure.h"
#include "fst/lib/compose.h"
#include "fst/lib/concat.h"
#include "fst/lib/determinize.h"
#include "fst/lib/difference.h"
#include "fst/lib/encode.h"
#include "fst/lib/epsnormalize.h"
#include "fst/lib/fst.h"
#include "fst/lib/intersect.h"
#include "fst/lib/invert.h"
#include "fst/lib/map.h"
#include "fst/lib/minimize.h"
#include "fst/lib/mutable-fst.h"
#include "fst/lib/project.h"
#include "fst/lib/prune.h"
#include "fst/lib/push.h"
#include "fst/lib/reverse.h"
#include "fst/lib/rmepsilon.h"
#include "fst/lib/shortest-path.h"
#include "fst/lib/symbol-table.h"
#include "fst/lib/union.h"
#include "fst/lib/vector-fst.h"
#include "fst/bin/draw-main.h"
