#ifndef _NRFRR_DEFS_H_
#define _NRFRR_DEFS_H_

#include <stdint.h>
#include <stdbool.h>
#include "nrfrr_conf.h"
#include "nrfrr_types.h"

#define RH_NRF51_AES_CCM_CNF_SIZE 33

#define FEM_TX()         do { if (_fem_tx >= 0) { digitalWrite(_fem_tx, HIGH); } if (_fem_rx >= 0) { digitalWrite(_fem_rx, LOW); }} while(0);
#define FEM_RX()         do { if (_fem_tx >= 0) { digitalWrite(_fem_tx, LOW); } if (_fem_rx >= 0) { digitalWrite(_fem_rx, HIGH); }} while(0);
#define FEM_OFF()        do { if (_fem_tx >= 0) { digitalWrite(_fem_tx, LOW); }} while (0)

#endif
