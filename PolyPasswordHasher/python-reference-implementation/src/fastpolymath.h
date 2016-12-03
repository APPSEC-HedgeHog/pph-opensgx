/* Author: Justin Cappos
 * File: fastpolymath.h
 * Purpose: The header file for the fastsimplexordatastore
 */

#include "Python.h"

typedef unsigned char gf256;

// Define all of the functions...

static PyObject *f(PyObject *module, PyObject *args);
static PyObject *full_lagrange(PyObject *module, PyObject *args);
static PyObject *py_pph_init_context(PyObject *module, PyObject *args);
static PyObject *py_pph_create_account(PyObject *module, PyObject *args);
static PyObject *py_pph_check_login(PyObject *module, PyObject *args);
static PyObject *py_pph_store_context(PyObject *module, PyObject *args);
static PyObject *py_pph_reload_context(PyObject *module, PyObject *args);
static PyObject *py_pph_unlock_password_data(PyObject *module, PyObject *args);
static PyObject *py_pph_destroy_context(PyObject *module, PyObject *args);

static void _multiply_polynomial_by_2terms_inplace(gf256 *dest,int length,
        gf256 terms[2]);

static void _multiply_polynomial_by_1term_inplace(gf256 *dest,int length,
        gf256 term);

static void _add_polynomials_inplace(gf256 *dest,int length,
        gf256 *terms);
