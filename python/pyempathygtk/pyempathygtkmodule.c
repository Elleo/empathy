#include <pygobject.h>

void empathy_register_classes (PyObject *d);
DL_EXPORT(void) initempathygtk(void);
extern PyMethodDef empathy_functions[];

DL_EXPORT(void) initempathygtk(void)
{
	PyObject *m, *d;

	init_pygobject ();
	
	m = Py_InitModule ("empathygtk", empathy_functions);
	d = PyModule_GetDict (m);
	
	empathy_register_classes (d);
	
	if (PyErr_Occurred ()) {
		PyErr_Print();
		Py_FatalError ("can't initialise module empathygtk");
	}
}

