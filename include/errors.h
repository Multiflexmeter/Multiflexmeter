#if !defined(_ERRORS_H_)
#define _ERRORS_H_

typedef enum {
  ERR_NONE,
  ERR_SMBUS_SLAVE_NACK,
  ERR_SMBUS_ARB_LOST,
  ERR_SMBUS_NO_ALERT,
  ERR_SMBUS_ERR,

} error_t;

void errorHandler(error_t err);

#endif // _ERRORS_H_