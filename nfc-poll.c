// Modifications by Dan Jackson, 2026
// File originally based on: https://github.com/nfc-tools/libnfc/blob/master/examples/nfc-poll.c

// Requires, on macOS:
//   brew install libnfc

// To compile, on macOS:
//   gcc -I$(brew --prefix)/include -L$(brew --prefix)/lib nfc-poll.c -o nfc-poll -lnfc

// To list devices -- alternatively:  $(brew --prefix)/bin/nfc-scan-device -v
//   ./nfc-poll -l

// To run:
//   ./nfc-poll $DEVICENAME
// ...outputs UID of detected cards, one per line, and an empty line once removed.  An initial empty line is sent if no card is present on startup.


// usb:072f:2200 -- location: usb:###:###


/*-
 * Free/Libre Near Field Communication (NFC) library
 *
 * Libnfc historical contributors:
 * Copyright (C) 2009      Roel Verdult
 * Copyright (C) 2009-2013 Romuald Conty
 * Copyright (C) 2010-2012 Romain Tartière
 * Copyright (C) 2010-2013 Philippe Teuwen
 * Copyright (C) 2012-2013 Ludovic Rousseau
 * See AUTHORS file for a more comprehensive list of contributors.
 * Additional contributors of this file:
 * Copyright (C) 2020      Adam Laurie
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *  1) Redistributions of source code must retain the above copyright notice,
 *  this list of conditions and the following disclaimer.
 *  2 )Redistributions in binary form must reproduce the above copyright
 *  notice, this list of conditions and the following disclaimer in the
 *  documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * Note that this license only applies on the examples, NFC library itself is under LGPL
 *
 */

/**
 * @file nfc-poll.c
 * @brief Polling example
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif // HAVE_CONFIG_H

#include <err.h>
#include <inttypes.h>
#include <signal.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include <nfc/nfc.h>
#include <nfc/nfc-types.h>

//#include "utils/nfc-utils.h"
#ifdef DEBUG
#  define ERR(...) do { \
    warnx ("ERROR %s:%d", __FILE__, __LINE__); \
    warnx ("    " __VA_ARGS__ ); \
  } while (0)
#else
#  define ERR(...)  warnx ("ERROR: " __VA_ARGS__ )
#endif
void
print_nfc_target(const nfc_target *pnt, bool verbose)
{
  char *s;
  str_nfc_target(&s, pnt, verbose);
  printf("%s", s);
  nfc_free(s);
}


#define MAX_UID_BYTES 10
size_t nfc_id(const nfc_target *pnt, const uint8_t **out_uid_pointer)
{
  const uint8_t *uid_pointer = NULL;
  size_t uid_size = 0;

  if (pnt != NULL) {
    switch (pnt->nm.nmt) {
      case NMT_ISO14443A:
      {
        const nfc_iso14443a_info *pnai = &pnt->nti.nai;
        uid_pointer = pnai->abtUid;
        uid_size = pnai->szUidLen;
        break;
      }
      case NMT_JEWEL:
      {
        const nfc_jewel_info *pnji = &pnt->nti.nji;
        uid_pointer = pnji->btId;
        uid_size = 4;
        break;
      }
      case NMT_BARCODE:
      {
        const nfc_barcode_info *pnti = &pnt->nti.nti;
        uid_pointer = pnti->abtData;
        uid_size = pnti->szDataLen;
        break;
      }
      case NMT_FELICA:
      {
        const nfc_felica_info *pnfi = &pnt->nti.nfi;
        uid_pointer = pnfi->abtId;
        uid_size = 8;
        break;
      }
      case NMT_ISO14443B:
      {
        const nfc_iso14443b_info *pnbi = &pnt->nti.nbi;
        uid_pointer = pnbi->abtPupi;
        uid_size = 4;
        break;
      }
      case NMT_ISO14443BI:
      {
        const nfc_iso14443bi_info *pnii = &pnt->nti.nii;
        uid_pointer = pnii->abtDIV;
        uid_size = 4;
        break;
      }
      case NMT_ISO14443B2SR:
      {
        const nfc_iso14443b2sr_info *pnsi = &pnt->nti.nsi;
        uid_pointer = pnsi->abtUID;
        uid_size = 8;
        break;
      }
      case NMT_ISO14443BICLASS:
      {
        const nfc_iso14443biclass_info *pnic = &pnt->nti.nhi;
        uid_pointer = pnic->abtUID;
        uid_size = 8;
        break;
      }
      case NMT_ISO14443B2CT:
      {
        const nfc_iso14443b2ct_info *pnci = &pnt->nti.nci;
        uid_pointer = pnci->abtUID;
        uid_size = sizeof(pnci->abtUID);
        break;
      }
      case NMT_DEP:
      {
        const nfc_dep_info *pndi = &pnt->nti.ndi;
        uid_pointer = pndi->abtNFCID3;
        uid_size = 10;
        break;
      }
    }
  }

  // Limit UID size
  if (uid_size > MAX_UID_BYTES) uid_size = MAX_UID_BYTES;
  if (out_uid_pointer != NULL) {
    *out_uid_pointer = uid_pointer;
  }
  return uid_size;
}


#define MAX_UID_STRING_BUFFER (MAX_UID_BYTES * 2 + 1)
char *nfc_id_string(const nfc_target *pnt, char *buffer)
{
  static char static_uid_buffer[MAX_UID_STRING_BUFFER];
  if (buffer == NULL) buffer = static_uid_buffer;

  const uint8_t *uid_pointer = NULL;
  size_t uid_size = nfc_id(pnt, &uid_pointer);
  if (uid_size > MAX_UID_BYTES) uid_size = MAX_UID_BYTES;
  if (uid_pointer == NULL) uid_size = 0;

  // Convert to hex string, in reverse order
  for (size_t i = 0; i < uid_size; i++) {
    sprintf(buffer + 2 * i, "%02x", pnt->nti.nai.abtUid[uid_size - 1 - i]);
  }

  // Pad remaining buffer with zeros (including terminating null)
  memset(buffer + 2 * uid_size, 0, MAX_UID_STRING_BUFFER - (2 * uid_size));

  // If no UID, return NULL
  if (uid_pointer == 0 || uid_size == 0) {
    return NULL;
  }
  return buffer;
}


#define MAX_DEVICE_COUNT 16

static nfc_device *pnd = NULL;
static nfc_context *context;

static void stop_polling(int sig)
{
  (void) sig;
  if (pnd != NULL)
    nfc_abort_command(pnd);
  else {
    nfc_exit(context);
    exit(EXIT_FAILURE);
  }
}

static void
print_usage(const char *progname)
{
  printf("usage: %s [-v] [-l] <device_name>\n", progname);
  printf("  -v\t verbose display\n");
  printf("  -l\t list devices\n");
}

int
main(int argc, const char *argv[])
{
  signal(SIGINT, stop_polling);

  bool help = false;
  bool verbose = false;
  bool list = false;
  int positional = 0;
  const char *arg_connection = NULL;
  for (int i = 1; i < argc; i++) {
    if (strcmp("-v", argv[i]) == 0) {
      verbose = true;
    } else if (strcmp("-l", argv[i]) == 0) {
      list = true;
    } else if (argv[i][0] == '-') {
      perror("Error: Unknown option.");
      help = true;
    } else {
      if (positional == 0) {
        arg_connection = argv[i];
      } else {
        perror("Error: Unknown positional argument.");
        help = true;
      }
      positional++;
    }
  }
  if (help) {
    print_usage(argv[0]);
    exit(EXIT_FAILURE);
  }

  const char *acLibnfcVersion = nfc_version();
  if (verbose) printf("%s uses libnfc %s\n", argv[0], acLibnfcVersion);

  const uint8_t uiPollNr = 1;   // 20
  const uint8_t uiPeriod = 2;
  const nfc_modulation nmModulations[] = {
    { .nmt = NMT_ISO14443A, .nbr = NBR_106 },
    { .nmt = NMT_ISO14443B, .nbr = NBR_106 },
    // { .nmt = NMT_FELICA, .nbr = NBR_212 },
    // { .nmt = NMT_FELICA, .nbr = NBR_424 },
    // { .nmt = NMT_JEWEL, .nbr = NBR_106 },
    { .nmt = NMT_ISO14443BICLASS, .nbr = NBR_106 },
  };
  const size_t szModulations = sizeof(nmModulations) / sizeof(nmModulations[0]);

  nfc_target nt;
  int res = 0;

  nfc_init(&context);
  if (context == NULL) {
    ERR("Unable to init libnfc (malloc)");
    exit(EXIT_FAILURE);
  }

  nfc_connstring connstrings[MAX_DEVICE_COUNT];
  size_t szDeviceFound = nfc_list_devices(context, connstrings, MAX_DEVICE_COUNT);
  if (verbose) printf("NFC device(s) found: %zu\n", szDeviceFound);
  // Match connection string
  int connectionIndex = -1;
  for (size_t i = 0; i < szDeviceFound; i++) {
    if (verbose) printf("#%zu: %s\n", i, connstrings[i]);
    if (arg_connection != NULL && strcmp(arg_connection, connstrings[i]) == 0) {
      connectionIndex = (int)i;
    }
  }
  // List devices and exit
  if (list) {
    for (size_t i = 0; i < szDeviceFound; i++) {
      printf("%s\n", connstrings[i]);
    }
    nfc_exit(context);
    exit(EXIT_SUCCESS);
  }

  if (connectionIndex < 0) {
    if (arg_connection != NULL) {
      ERR("NFC device with connection string '%s' not found.", arg_connection);
      nfc_exit(context);
      exit(EXIT_FAILURE);
    }
    if (verbose) printf("Note: No connection string specified.\n");
  }

  pnd = nfc_open(context, connectionIndex >= 0 ? connstrings[connectionIndex] : NULL);

  if (pnd == NULL) {
    ERR("%s", "Unable to open NFC device.");
    nfc_exit(context);
    exit(EXIT_FAILURE);
  }

  if (nfc_initiator_init(pnd) < 0) {
    nfc_perror(pnd, "nfc_initiator_init");
    nfc_close(pnd);
    nfc_exit(context);
    exit(EXIT_FAILURE);
  }

  if (verbose) printf("NFC reader: %s opened\n", nfc_device_get_name(pnd));

  bool report_initial_empty = true;
rescan:
  if (verbose) printf("NFC device will poll during %ld ms (%u pollings of %lu ms for %" PRIdPTR " modulations)\n", (unsigned long) uiPollNr * szModulations * uiPeriod * 150, uiPollNr, (unsigned long) uiPeriod * 150, szModulations);
  if ((res = nfc_initiator_poll_target(pnd, nmModulations, szModulations, uiPollNr, uiPeriod, &nt))  < 0) {
    nfc_perror(pnd, "nfc_initiator_poll_target");
    nfc_close(pnd);
    nfc_exit(context);
    exit(EXIT_FAILURE);
  }

  if (res > 0) {
    // Card detected
    char *id = nfc_id_string(&nt, NULL);
    if (id != NULL) {
      printf("%s\n", id);
    } else {
      printf("?\n");
    }
    if (verbose) print_nfc_target(&nt, verbose);
    if (verbose) printf("Waiting for card removing...");
    fflush(stdout);

    // Pause until card is removed
    while (!nfc_initiator_target_is_present(pnd, NULL)) {
      ; // TODO: Ensure this is interruptable with Ctrl+C
    }
    if (verbose) nfc_perror(pnd, "nfc_initiator_target_is_present");
    if (verbose) printf("done.\n");

    // Card removed
    printf("\n");
    fflush(stdout);
  } else {
    if (report_initial_empty) {
      // Initial state empty report
      printf("\n");
      fflush(stdout);
    }
    if (verbose) printf("No target found.\n");
  }
  report_initial_empty = false;

  goto rescan;

  // nfc_close(pnd);
  // nfc_exit(context);
  // exit(EXIT_SUCCESS);
}
