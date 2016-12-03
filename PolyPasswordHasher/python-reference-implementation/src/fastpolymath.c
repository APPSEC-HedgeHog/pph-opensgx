/* 
<Author>
  Justin Cappos

<Start Date>
  Apr 1st, 2013

<Description>
  A wrapper for fast C operations for polynomial math for PolyPasswordHasher.
  Specifically, I have two external interfaces: computing f(x) and
  doing full Lagrange interpolation.   I do this for polynomials in GF256
  using x^8 + x^4 + x^3 + x + 1 as multiplication and XOR for addition.   
  This was chosen because it's used in tss and AES.

 */

#include "Python.h"
#include "fastpolymath.h"
#include "libpolypasswordhasher.h"




// ***********************  GF256 helpers     ************************

// Will use for fast math...
static const gf256 _GF256_EXP[256] = 
      {0x01, 0x03, 0x05, 0x0f, 0x11, 0x33, 0x55, 0xff,
       0x1a, 0x2e, 0x72, 0x96, 0xa1, 0xf8, 0x13, 0x35,
       0x5f, 0xe1, 0x38, 0x48, 0xd8, 0x73, 0x95, 0xa4,
       0xf7, 0x02, 0x06, 0x0a, 0x1e, 0x22, 0x66, 0xaa,
       0xe5, 0x34, 0x5c, 0xe4, 0x37, 0x59, 0xeb, 0x26,
       0x6a, 0xbe, 0xd9, 0x70, 0x90, 0xab, 0xe6, 0x31,
       0x53, 0xf5, 0x04, 0x0c, 0x14, 0x3c, 0x44, 0xcc,
       0x4f, 0xd1, 0x68, 0xb8, 0xd3, 0x6e, 0xb2, 0xcd,
       0x4c, 0xd4, 0x67, 0xa9, 0xe0, 0x3b, 0x4d, 0xd7,
       0x62, 0xa6, 0xf1, 0x08, 0x18, 0x28, 0x78, 0x88,
       0x83, 0x9e, 0xb9, 0xd0, 0x6b, 0xbd, 0xdc, 0x7f,
       0x81, 0x98, 0xb3, 0xce, 0x49, 0xdb, 0x76, 0x9a,
       0xb5, 0xc4, 0x57, 0xf9, 0x10, 0x30, 0x50, 0xf0,
       0x0b, 0x1d, 0x27, 0x69, 0xbb, 0xd6, 0x61, 0xa3,
       0xfe, 0x19, 0x2b, 0x7d, 0x87, 0x92, 0xad, 0xec,
       0x2f, 0x71, 0x93, 0xae, 0xe9, 0x20, 0x60, 0xa0,
       0xfb, 0x16, 0x3a, 0x4e, 0xd2, 0x6d, 0xb7, 0xc2,
       0x5d, 0xe7, 0x32, 0x56, 0xfa, 0x15, 0x3f, 0x41,
       0xc3, 0x5e, 0xe2, 0x3d, 0x47, 0xc9, 0x40, 0xc0,
       0x5b, 0xed, 0x2c, 0x74, 0x9c, 0xbf, 0xda, 0x75,
       0x9f, 0xba, 0xd5, 0x64, 0xac, 0xef, 0x2a, 0x7e,
       0x82, 0x9d, 0xbc, 0xdf, 0x7a, 0x8e, 0x89, 0x80,
       0x9b, 0xb6, 0xc1, 0x58, 0xe8, 0x23, 0x65, 0xaf,
       0xea, 0x25, 0x6f, 0xb1, 0xc8, 0x43, 0xc5, 0x54,
       0xfc, 0x1f, 0x21, 0x63, 0xa5, 0xf4, 0x07, 0x09,
       0x1b, 0x2d, 0x77, 0x99, 0xb0, 0xcb, 0x46, 0xca,
       0x45, 0xcf, 0x4a, 0xde, 0x79, 0x8b, 0x86, 0x91,
       0xa8, 0xe3, 0x3e, 0x42, 0xc6, 0x51, 0xf3, 0x0e,
       0x12, 0x36, 0x5a, 0xee, 0x29, 0x7b, 0x8d, 0x8c,
       0x8f, 0x8a, 0x85, 0x94, 0xa7, 0xf2, 0x0d, 0x17,
       0x39, 0x4b, 0xdd, 0x7c, 0x84, 0x97, 0xa2, 0xfd,
       0x1c, 0x24, 0x6c, 0xb4, 0xc7, 0x52, 0xf6, 0x01};


// entry 0 is undefined
static const gf256 _GF256_LOG[256] = 
       {0x00, 0x00, 0x19, 0x01, 0x32, 0x02, 0x1a, 0xc6,
        0x4b, 0xc7, 0x1b, 0x68, 0x33, 0xee, 0xdf, 0x03,
        0x64, 0x04, 0xe0, 0x0e, 0x34, 0x8d, 0x81, 0xef,
        0x4c, 0x71, 0x08, 0xc8, 0xf8, 0x69, 0x1c, 0xc1,
        0x7d, 0xc2, 0x1d, 0xb5, 0xf9, 0xb9, 0x27, 0x6a,
        0x4d, 0xe4, 0xa6, 0x72, 0x9a, 0xc9, 0x09, 0x78,
        0x65, 0x2f, 0x8a, 0x05, 0x21, 0x0f, 0xe1, 0x24,
        0x12, 0xf0, 0x82, 0x45, 0x35, 0x93, 0xda, 0x8e,
        0x96, 0x8f, 0xdb, 0xbd, 0x36, 0xd0, 0xce, 0x94,
        0x13, 0x5c, 0xd2, 0xf1, 0x40, 0x46, 0x83, 0x38,
        0x66, 0xdd, 0xfd, 0x30, 0xbf, 0x06, 0x8b, 0x62,
        0xb3, 0x25, 0xe2, 0x98, 0x22, 0x88, 0x91, 0x10,
        0x7e, 0x6e, 0x48, 0xc3, 0xa3, 0xb6, 0x1e, 0x42,
        0x3a, 0x6b, 0x28, 0x54, 0xfa, 0x85, 0x3d, 0xba,
        0x2b, 0x79, 0x0a, 0x15, 0x9b, 0x9f, 0x5e, 0xca,
        0x4e, 0xd4, 0xac, 0xe5, 0xf3, 0x73, 0xa7, 0x57,
        0xaf, 0x58, 0xa8, 0x50, 0xf4, 0xea, 0xd6, 0x74,
        0x4f, 0xae, 0xe9, 0xd5, 0xe7, 0xe6, 0xad, 0xe8,
        0x2c, 0xd7, 0x75, 0x7a, 0xeb, 0x16, 0x0b, 0xf5,
        0x59, 0xcb, 0x5f, 0xb0, 0x9c, 0xa9, 0x51, 0xa0,
        0x7f, 0x0c, 0xf6, 0x6f, 0x17, 0xc4, 0x49, 0xec,
        0xd8, 0x43, 0x1f, 0x2d, 0xa4, 0x76, 0x7b, 0xb7,
        0xcc, 0xbb, 0x3e, 0x5a, 0xfb, 0x60, 0xb1, 0x86,
        0x3b, 0x52, 0xa1, 0x6c, 0xaa, 0x55, 0x29, 0x9d,
        0x97, 0xb2, 0x87, 0x90, 0x61, 0xbe, 0xdc, 0xfc,
        0xbc, 0x95, 0xcf, 0xcd, 0x37, 0x3f, 0x5b, 0xd1,
        0x53, 0x39, 0x84, 0x3c, 0x41, 0xa2, 0x6d, 0x47,
        0x14, 0x2a, 0x9e, 0x5d, 0x56, 0xf2, 0xd3, 0xab,
        0x44, 0x11, 0x92, 0xd9, 0x23, 0x20, 0x2e, 0x89,
        0xb4, 0x7c, 0xb8, 0x26, 0x77, 0x99, 0xe3, 0xa5,
        0x67, 0x4a, 0xed, 0xde, 0xc5, 0x31, 0xfe, 0x18,
        0x0d, 0x63, 0x8c, 0x80, 0xc0, 0xf7, 0x70, 0x07};


// Multiplicative inverse... The first entry is junk because 0 has no inverse.
static const gf256 _GF256_INV[256] = 
       {0x00, 0x01, 0x8d, 0xf6, 0xcb, 0x52, 0x7b, 0xd1, 
        0xe8, 0x4f, 0x29, 0xc0, 0xb0, 0xe1, 0xe5, 0xc7,
        0x74, 0xb4, 0xaa, 0x4b, 0x99, 0x2b, 0x60, 0x5f,
        0x58, 0x3f, 0xfd, 0xcc, 0xff, 0x40, 0xee, 0xb2,
        0x3a, 0x6e, 0x5a, 0xf1, 0x55, 0x4d, 0xa8, 0xc9,
        0xc1, 0x0a, 0x98, 0x15, 0x30, 0x44, 0xa2, 0xc2,
        0x2c, 0x45, 0x92, 0x6c, 0xf3, 0x39, 0x66, 0x42,
        0xf2, 0x35, 0x20, 0x6f, 0x77, 0xbb, 0x59, 0x19,
        0x1d, 0xfe, 0x37, 0x67, 0x2d, 0x31, 0xf5, 0x69,
        0xa7, 0x64, 0xab, 0x13, 0x54, 0x25, 0xe9, 0x09,
        0xed, 0x5c, 0x05, 0xca, 0x4c, 0x24, 0x87, 0xbf,
        0x18, 0x3e, 0x22, 0xf0, 0x51, 0xec, 0x61, 0x17,
        0x16, 0x5e, 0xaf, 0xd3, 0x49, 0xa6, 0x36, 0x43,
        0xf4, 0x47, 0x91, 0xdf, 0x33, 0x93, 0x21, 0x3b,
        0x79, 0xb7, 0x97, 0x85, 0x10, 0xb5, 0xba, 0x3c,
        0xb6, 0x70, 0xd0, 0x06, 0xa1, 0xfa, 0x81, 0x82,
        0x83, 0x7e, 0x7f, 0x80, 0x96, 0x73, 0xbe, 0x56,
        0x9b, 0x9e, 0x95, 0xd9, 0xf7, 0x02, 0xb9, 0xa4,
        0xde, 0x6a, 0x32, 0x6d, 0xd8, 0x8a, 0x84, 0x72,
        0x2a, 0x14, 0x9f, 0x88, 0xf9, 0xdc, 0x89, 0x9a,
        0xfb, 0x7c, 0x2e, 0xc3, 0x8f, 0xb8, 0x65, 0x48,
        0x26, 0xc8, 0x12, 0x4a, 0xce, 0xe7, 0xd2, 0x62,
        0x0c, 0xe0, 0x1f, 0xef, 0x11, 0x75, 0x78, 0x71,
        0xa5, 0x8e, 0x76, 0x3d, 0xbd, 0xbc, 0x86, 0x57,
        0x0b, 0x28, 0x2f, 0xa3, 0xda, 0xd4, 0xe4, 0x0f,
        0xa9, 0x27, 0x53, 0x04, 0x1b, 0xfc, 0xac, 0xe6,
        0x7a, 0x07, 0xae, 0x63, 0xc5, 0xdb, 0xe2, 0xea,
        0x94, 0x8b, 0xc4, 0xd5, 0x9d, 0xf8, 0x90, 0x6b,
        0xb1, 0x0d, 0xd6, 0xeb, 0xc6, 0x0e, 0xcf, 0xad,
        0x08, 0x4e, 0xd7, 0xe3, 0x5d, 0x50, 0x1e, 0xb3,
        0x5b, 0x23, 0x38, 0x34, 0x68, 0x46, 0x03, 0x8c,
        0xdd, 0x9c, 0x7d, 0xa0, 0xcd, 0x1a, 0x41, 0x1c};





// only a function for code clarity
static inline gf256 _gf256_add(gf256 a, gf256 b) {
  return a^b;
}

// only a function for code clarity
static inline gf256 _gf256_sub(gf256 a, gf256 b) {
  return a^b;
}


static inline gf256 _gf256_mul(gf256 a, gf256 b) {
  if ((a==0)||(b==0)) {
    return 0;
  }
  return _GF256_EXP[(_GF256_LOG[a] + _GF256_LOG[b]) % 255];
}


static inline gf256 _gf256_div(gf256 a, gf256 b) {
  if (a==0) {
    return 0;
  }
  if (b==0) {
    printf("Error!   Cannot divide by zero!\n");
    exit(1);
  }
  // mod in C will return a negative!
  return _GF256_EXP[(255+_GF256_LOG[a] - _GF256_LOG[b]) % 255];
}








// ***********************  main code     ************************







// Python wrapper...
static PyObject *f(PyObject *module, PyObject *args) {
  gf256 x;
  gf256 *coefs_bytes;
  int coefs_length;

  if (!PyArg_ParseTuple(args, "cs#", &x,&coefs_bytes,&coefs_length)) {
    // Incorrect args...
    return NULL;
  }

  // The share should not be 0.
  if (x == 0) {
    PyErr_SetString(PyExc_ValueError, "invalid share index value, cannot be 0");
    return NULL;
  }

  gf256 accumulator;
  gf256 x_i;
  int i;

  accumulator = 0;
  // start with x_i = 1.   We'll multiply by x each time around to increase it.
  x_i = 1;
  for (i=0; i<coefs_length; i++) {
    // we multiply this byte (a,b, or c) with x raised to the right power.
    accumulator = _gf256_add(accumulator, _gf256_mul(coefs_bytes[i], x_i));
    // raise x_i to the next power by multiplying by x.
    x_i = _gf256_mul(x_i, x);
  }

  return Py_BuildValue("i",accumulator);

}

static PyObject *py_pph_init_context(PyObject *module, PyObject *args)
{
  int threshold;
  int isolated_check_bits;

  if (!PyArg_ParseTuple(args, "ii", &threshold,&isolated_check_bits)) {
    // Incorrect args...
    return NULL;
  }
  pph_context *context;
  context = pph_init_context(threshold, isolated_check_bits);
  return Py_BuildValue("i",context);
}

static PyObject *py_pph_create_account(PyObject *module, PyObject *args)
{
  char * username;
  char * password;
  int *username_length;
  int *password_length;
  unsigned int *ptr;
  int *shares;

  if (!PyArg_ParseTuple(args, "is#s#i", &ptr, &username, &username_length, &password, &password_length, &shares)) {
    // Incorrect args...
    return NULL;
  }
  printf("username [%s] passpwrd [%s] username_length [%d] password_length [%d] \n",username,password,username_length, password_length);
  pph_context *context = (pph_context *) ptr;
  pph_create_account(context, username, username_length, password, password_length, shares);
  return Py_BuildValue("i",0);
}

static PyObject *py_pph_check_login(PyObject *module, PyObject *args)
{
  char * username;
  char * password;
  int *username_length;
  int *password_length;
  unsigned int *ptr;

  if (!PyArg_ParseTuple(args, "is#s#", &ptr, &username, &username_length, &password, &password_length)) {
    // Incorrect args...
    return NULL;
  }
  printf("Check username [%s] passpwrd [%s] username_length [%d] password_length [%d] \n",username,password,username_length, password_length);
  pph_context *context = (pph_context *) ptr;
  int ret = pph_check_login(context, username, username_length, password, password_length);
  if(ret==0)
    printf("Successful login !\n");
  else
    printf("Unsuccesful login !\n");
  return Py_BuildValue("i",ret);
}

static PyObject *py_pph_store_context(PyObject *module, PyObject *args)
{
  char * file_name;
  unsigned int *ptr;

  printf("py_pph_store_context b4  \n");
  if (!PyArg_ParseTuple(args, "is", &ptr, &file_name)) {
    // Incorrect args...
    printf("incorrect args \n");
    return NULL;
  }
  printf("Password file name is [%s] ",file_name);

  pph_context *context = (pph_context *) ptr;
  int ret = pph_store_context(context, file_name);
  if(ret !=0 )
    printf("Error saving passwords file \n");

  return Py_BuildValue("i",ret);
}

static PyObject *py_pph_reload_context(PyObject *module, PyObject *args)
{
  char * file_name;
  
  if (!PyArg_ParseTuple(args, "s", &file_name)) {
    // Incorrect args...
    return NULL;
  }
  
  printf("py_pph_reload_context \n");
  pph_context *context;
  context = pph_reload_context(file_name);
  return Py_BuildValue("i",context);
}

static PyObject *py_pph_unlock_password_data(PyObject *module, PyObject *args)
{
  PyObject * listObj;
  PyObject * listObj1;
  PyObject * stringObj;

  int size;
  int idx;
  unsigned int ptr;

  char **username = malloc(sizeof (*username) * size);
  char **password = malloc(sizeof (*password) * size);
  int **username_length = malloc(sizeof (*username_length) * size);
  int **password_length = malloc(sizeof (*password_length) * size);

  printf("calliing unlock \n");


  if(!PyArg_ParseTuple(args, "iO!O!", &ptr, &PyList_Type,  &listObj, &PyList_Type, &listObj1))
    return NULL;

  size = PyList_Size(listObj);  

  for(idx=0;idx<size;idx++)
  {
    stringObj=PyList_GetItem(listObj,idx);
    username[idx] =PyString_AsString(stringObj);
    stringObj=PyList_GetItem(listObj1,idx);
    password[idx] =PyString_AsString(stringObj);
    username_length[idx]= strlen(username[idx]);
    password_length[idx]= strlen(password[idx]);

    printf("Usernames [%s] \n",username[idx]);
    printf("Passwords [%s] \n",password[idx]);
  }

  printf("context [%d] \n",ptr);
  pph_context *context = (pph_context *) ptr;
  int ret =pph_unlock_password_data(context,size, username, username_length, password, password_length );
  if(ret != 0)
    printf("Unable to unlock database[%d]\n",ret);

  // free(username);
  // free(password);
  // free(username_length);
  // free(password_length);

  return Py_BuildValue("i",ret);



  // if(!PyArg_ParseTuple(args, "O!", &PyList_Type,  &listObj))
  //   return NULL;

  // for(idx=0;idx<size;idx++)
  // {
  //   stringObj=PyList_GetItem(listObj1,idx);
  //   password[idx] =PyString_AsString(stringObj);
  //   printf("Passwords [%s] \n",password[idx]);
  // }


}

static PyObject *full_lagrange(PyObject *module, PyObject *args) {
  // The args are xs (an array of gf256s) and fxs (also an array of gf256s).  
 
  gf256 *xs;  
  gf256 *fxs;  
  int length;
  int i, j;

  // 256 because this is the max shares possible in GF256
  // This is copied out, so it's okay this is a local variable.
  gf256 returnedcoefficients[256];
  gf256 temppolynomial[256];

  if (!PyArg_ParseTuple(args, "ss#", &xs,&fxs, &length)) {
    // Incorrect args...
    return NULL;
  }


  // clear out the returned area
  for (i=0; i<length; i++){
    returnedcoefficients[i] = 0;
  }

  gf256 denominator;

  // This will hold one term...
  gf256 this_term[2];

  // we need to compute:
  // l_0 =  (x - x_1) / (x_0 - x_1)   *   (x - x_2) / (x_0 - x_2) * ...
  // l_1 =  (x - x_0) / (x_1 - x_0)   *   (x - x_2) / (x_1 - x_2) * ...
  // ...

  for (i=0; i<length;i++) {
    // Zero this out (except the first position).   I'm multiplying and
    // accumulating the value so want to start with 1, not 0.
    temppolynomial[0] = 1;
    for (j=1; j<length;j++) {
      temppolynomial[j] = 0;
    }

    // take the terms one at a time.
    // I'll calculate the denominator and use it to compute the polynomial.
    for (j=0; j<length; j++) {

      // Skip i=j because that's how Lagrange works...
      if (i==j) {
        continue;
      }

      denominator = _gf256_sub(xs[i],xs[j]);

      this_term[0] = _gf256_div(xs[j],denominator);
      // Precomputed the table of inverses
//      this_term[1] = _gf256_div(1,denominator);
      this_term[1] = _GF256_INV[denominator];

      // Now, build the polynomial...
      _multiply_polynomial_by_2terms_inplace(temppolynomial,length,this_term);
    }
    // okay, now I've gone and computed the polynomial.   I need to multiply it
    // by the result of f(x)

    _multiply_polynomial_by_1term_inplace(temppolynomial,length,fxs[i]);

    _add_polynomials_inplace(returnedcoefficients,length,temppolynomial);
    
  }

  // This is copied out, so it's okay this is a local variable.
  PyObject *return_str_obj = Py_BuildValue("s#",returnedcoefficients,length);

  return return_str_obj;

}



static void _multiply_polynomial_by_2terms_inplace(gf256 *dest,int length,
        gf256 terms[2]) {

  int i;
  // I need a buffer to keep the temp data and don't want to malloc / free.
  gf256 tempbuffer[256];
  // should not be possible because it would overflow...
  if (dest[length-1] != 0) {
    printf("The destination contains %d!\n",dest[length-1]);
    printf("others %d, %d!\n",dest[0],dest[1]);
    exit(1);
  }

  // put the upper term * dest in the temp buffer 
  for (i=0; i<length; i++) {
    tempbuffer[i] = _gf256_mul(dest[i],terms[1]);
  }

  // compute the lower term times the dest...
  _multiply_polynomial_by_1term_inplace(dest,length,terms[0]);

  // Now add them, but offset the temp buffer by 1 because it was multiplied
  // by the upper term...
  for (i=0; i<length-1; i++) {
    dest[i+1] = _gf256_add(tempbuffer[i],dest[i+1]);
  }

}

static void _multiply_polynomial_by_1term_inplace(gf256 *dest,int length,
        gf256 term) {
  int i;
  for (i=0;i<length;i++) {
    dest[i] = _gf256_mul(dest[i],term);
  }
}

static void _add_polynomials_inplace(gf256 *dest,int length,gf256 *terms) {
  int i;
  for (i=0;i<length;i++) {
    dest[i] = _gf256_add(dest[i],terms[i]);
  }
}




// ******************* Code to make us a Python module *******************

static PyMethodDef MyFastPolyMathMethods [] = {
  {"f", f, METH_VARARGS, "Compute f(x)."},
  {"full_lagrange", full_lagrange, METH_VARARGS, 
      "Return the full lagrange for a set of shares."},
  {"py_pph_init_context", py_pph_init_context, METH_VARARGS, 
      "Init pph context"},
  {"py_pph_create_account", py_pph_create_account, METH_VARARGS, 
      "Create pph account"},
  {"py_pph_check_login", py_pph_check_login, METH_VARARGS, 
      "Check login"}, 
  {"py_pph_store_context", py_pph_store_context, METH_VARARGS, 
      "Stotre context"},
  {"py_pph_reload_context", py_pph_reload_context, METH_VARARGS, 
      "Reload context"},
  {"py_pph_unlock_password_data", py_pph_unlock_password_data, METH_VARARGS, 
      "Unlock password "},
  {NULL, NULL, 0, NULL}
};


PyMODINIT_FUNC initfastpolymath_c(void) {
  Py_InitModule("fastpolymath_c", MyFastPolyMathMethods);
}


