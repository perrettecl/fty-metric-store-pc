/*  =========================================================================
    actor_commands - actor commands

    Copyright (C) 2014 - 2017 Eaton

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
    =========================================================================
*/

/*
@header
    actor_commands - actor commands
@discuss
@end
*/

#include "fty_metric_store_classes.h"

int
actor_commands (mlm_client_t *client, zmsg_t **message_p)
{
    assert (message_p && *message_p);
    zmsg_t *message = *message_p;

    char *cmd = zmsg_popstr (message);
    if (!cmd) {
        log_error (
                "Given `which == pipe` function `zmsg_popstr (msg)` returned NULL. "
                "Message received is most probably empty (has no frames).");
        zmsg_destroy (message_p);
        return 0;
    }

    log_debug ("actor command = '%s'", cmd);

    if (streq (cmd, "$TERM")) {
        log_info ("Got $TERM");
        zstr_free (&cmd);
        zmsg_destroy (message_p);
        return 1; // special $TERM rv
    }

    if (streq (cmd, "CONNECT")) {
        char *endpoint = zmsg_popstr (message);
        char *name = zmsg_popstr (message);

        if (!endpoint) {
            log_error (
                "Expected multipart string format: CONNECT/endpoint/name. "
                "Received CONNECT/nullptr");
        }
        else if (!name) {
            log_error (
                "Expected multipart string format: CONNECT/endpoint/name. "
                "Received CONNECT/endpoint/nullptr");
        }
        else {
            int rv = mlm_client_connect (client, endpoint, 1000, name);
            if (rv == -1) {
                log_error (
                        "mlm_client_connect (endpoint = '%s', timeout = '%d', address = '%s') failed",
                        endpoint, 1000, name);
            }
        }

        zstr_free (&name);
        zstr_free (&endpoint);
    }
    else if (streq (cmd, "PRODUCER")) {
        char *stream = zmsg_popstr (message);

        if (!stream) {
            log_error (
                    "Expected multipart string format: PRODUCER/stream. "
                    "Received PRODUCER/nullptr");
        }
        else {
            int rv = mlm_client_set_producer (client, stream);
            if (rv == -1) {
                log_error ("mlm_client_set_producer (stream = '%s') failed", stream);
            }
        }

        zstr_free (&stream);
    }
    else if (streq (cmd, "CONSUMER")) {
        char *stream = zmsg_popstr (message);
        char *pattern = zmsg_popstr (message);

        if (!stream) {
            log_error (
                    "Expected multipart string format: CONSUMER/stream/pattern. "
                    "Received CONSUMER/nullptr");
        }
        else if (!pattern) {
            log_error (
                    "Expected multipart string format: CONSUMER/stream/pattern. "
                    "Received CONSUMER/stream/nullptr");
        }
        else {
            int rv = mlm_client_set_consumer (client, stream, pattern);
            if (rv == -1) {
                log_error (
                        "mlm_client_set_consumer (stream = '%s', pattern = '%s') failed",
                        stream, pattern);
            }
        }

        zstr_free (&pattern);
        zstr_free (&stream);
    }
    else if (streq (cmd, "CONFIGURE")) {
        char *config_file = zmsg_popstr (message);
        if (!config_file) {
            log_error (
                    "Expected multipart string format: CONFIGURE/config_file. "
                    "Received CONFIGURE/nullptr");
        }
        else {
            log_warning("TODO: implement config file");
        }

        zstr_free (&config_file);
    }
    else if (streq (cmd, FTY_METRIC_STORE_CONF_PREFIX)) {
        log_debug ("%s is not yet implemented!", FTY_METRIC_STORE_CONF_PREFIX);
    }
    else {
        log_warning ("Command '%s' is unknown or not implemented", cmd);
    }

    zstr_free (&cmd);
    zmsg_destroy (message_p);

    return 0;
}

//  --------------------------------------------------------------------------
//  Self test of this class

#define STDERR_EMPTY \
    {\
    fseek (fp, 0L, SEEK_END);\
    uint64_t sz = ftell (fp);\
    fclose (fp);\
    if (sz > 0) { int r; printf ("FATAL: stderr of last operation was not empty:\n"); r = system( ("cat " + str_stderr_txt).c_str() ); assert (r==0); };\
    assert (sz == 0);\
    }

#define STDERR_NON_EMPTY \
    {\
    fseek (fp, 0L, SEEK_END);\
    uint64_t sz = ftell (fp);\
    fclose (fp);\
    if (sz == 0) { printf ("FATAL: stderr of last operation was empty while expected to have an error log!\n"); };\
    assert (sz > 0);\
    }

void
actor_commands_test (bool verbose)
{
    // Note: If your selftest reads SCMed fixture data, please keep it in
    // src/selftest-ro; if your test creates filesystem objects, please
    // do so under src/selftest-rw. They are defined below along with a
    // usecase (asert) to make compilers happy.
    const char *SELFTEST_DIR_RO = "src/selftest-ro";
    const char *SELFTEST_DIR_RW = "src/selftest-rw";
    assert (SELFTEST_DIR_RO);
    assert (SELFTEST_DIR_RW);
    std::string str_SELFTEST_DIR_RO = std::string(SELFTEST_DIR_RO);
    std::string str_SELFTEST_DIR_RW = std::string(SELFTEST_DIR_RW);
    std::string str_stderr_txt = str_SELFTEST_DIR_RW + "/stderr.txt";

    printf (" * actor_commands: ");
    ManageFtyLog::setInstanceFtylog("actor_commands");
    // since this test suite checks stderr, log on WARNING level
    ManageFtyLog::getInstanceFtylog()->setLogLevelWarning();
    //  @selftest
    static const char* endpoint = "ipc://ms-test-actor-commands";

    // malamute broker
    zactor_t *malamute = zactor_new (mlm_server, (void*) "Malamute");
    assert (malamute);
    if (verbose)
        zstr_send (malamute, "VERBOSE");
    zstr_sendx (malamute, "BIND", endpoint, NULL);

    mlm_client_t *client = mlm_client_new ();
    assert (client);

    zmsg_t *message = NULL;


    // --------------------------------------------------------------
    FILE *fp = freopen (str_stderr_txt.c_str(), "w+", stderr);
    // empty message - expected fail
    message = zmsg_new ();
    assert (message);
    int rv = actor_commands (client, &message);
    assert (rv == 0);
    assert (message == NULL);

    STDERR_NON_EMPTY

    // --------------------------------------------------------------
    fp = freopen (str_stderr_txt.c_str(), "w+", stderr);
    // empty string - expected fail
    message = zmsg_new ();
    assert (message);
    zmsg_addstr (message, "");
    rv = actor_commands (client, &message);
    assert (rv == 0);
    assert (message == NULL);

    STDERR_NON_EMPTY

    // --------------------------------------------------------------
    fp = freopen (str_stderr_txt.c_str(), "w+", stderr);
    // unknown command - expected fail
    message = zmsg_new ();
    assert (message);
    zmsg_addstr (message, "MAGIC!");
    rv = actor_commands (client, &message);
    assert (rv == 0);
    assert (message == NULL);

    STDERR_NON_EMPTY

    // --------------------------------------------------------------
    fp = freopen (str_stderr_txt.c_str(), "w+", stderr);
    // CONFIGURE - expected fail
    message = zmsg_new ();
    assert (message);
    zmsg_addstr (message, "CONFIGURE");
    // missing config_file here
    rv = actor_commands (client, &message);
    assert (rv == 0);
    assert (message == NULL);

    STDERR_NON_EMPTY

    // --------------------------------------------------------------
/* TODO: uncomment test when CONFIGURE functionality implemented

    fp = freopen (str_stderr_txt.c_str(), "w+", stderr);
    // CONFIGURE
    message = zmsg_new ();
    assert (message);
    zmsg_addstr (message, "CONFIGURE");
    zmsg_addstr (message, "mapping.conf");
    rv = actor_commands (client, &message);
    assert (rv == 0);
    assert (message == NULL);

    STDERR_EMPTY

*/
    // --------------------------------------------------------------
    fp = freopen (str_stderr_txt.c_str(), "w+", stderr);
    // CONNECT - expected fail
    message = zmsg_new ();
    assert (message);
    zmsg_addstr (message, "CONNECT");
    zmsg_addstr (message, endpoint);
    // missing name here
    rv = actor_commands (client, &message);
    assert (rv == 0);
    assert (message == NULL);

    STDERR_NON_EMPTY

    // --------------------------------------------------------------
    fp = freopen (str_stderr_txt.c_str(), "w+", stderr);
    // CONNECT - expected fail
    message = zmsg_new ();
    assert (message);
    zmsg_addstr (message, "CONNECT");
    // missing endpoint here
    // missing name here
    rv = actor_commands (client, &message);
    assert (rv == 0);
    assert (message == NULL);

    STDERR_NON_EMPTY

    // --------------------------------------------------------------
    fp = freopen (str_stderr_txt.c_str(), "w+", stderr);
    // CONNECT - expected fail; bad endpoint
    message = zmsg_new ();
    assert (message);
    zmsg_addstr (message, "CONNECT");
    zmsg_addstr (message, "ipc://bios-ws-server-BAD");
    zmsg_addstr (message, "test-agent");
    rv = actor_commands (client, &message);
    assert (rv == 0);
    assert (message == NULL);

    STDERR_NON_EMPTY

    // The original client still waiting on the bad endpoint for malamute
    // server to show up. Therefore we must destroy and create it again.
    mlm_client_destroy (&client);
    client = mlm_client_new ();
    assert (client);

    // --------------------------------------------------------------
    fp = freopen (str_stderr_txt.c_str(), "w+", stderr);

    // $TERM
    message = zmsg_new ();
    assert (message);
    zmsg_addstr (message, "$TERM");
    rv = actor_commands (client, &message);
    assert (rv == 1);
    assert (message == NULL);

    // CONNECT
    message = zmsg_new ();
    assert (message);
    zmsg_addstr (message, "CONNECT");
    zmsg_addstr (message, endpoint);
    zmsg_addstr (message, "test-agent");
    rv = actor_commands (client, &message);
    assert (rv == 0);
    assert (message == NULL);

    // CONSUMER
    message = zmsg_new ();
    assert (message);
    zmsg_addstr (message, "CONSUMER");
    zmsg_addstr (message, "some-stream");
    zmsg_addstr (message, ".+@.+");
    rv = actor_commands (client, &message);
    assert (rv == 0);
    assert (message == NULL);

    // PRODUCER
    message = zmsg_new ();
    assert (message);
    zmsg_addstr (message, "PRODUCER");
    zmsg_addstr (message, "some-stream");
    rv = actor_commands (client, &message);
    assert (rv == 0);
    assert (message == NULL);

    STDERR_EMPTY

    // --------------------------------------------------------------
    fp = freopen (str_stderr_txt.c_str(), "w+", stderr);
    // CONSUMER - expected fail
    message = zmsg_new ();
    assert (message);
    zmsg_addstr (message, "CONSUMER");
    zmsg_addstr (message, "some-stream");
    // missing pattern here
    rv = actor_commands (client, &message);
    assert (rv == 0);
    assert (message == NULL);

    STDERR_NON_EMPTY

    // --------------------------------------------------------------
    fp = freopen (str_stderr_txt.c_str(), "w+", stderr);
    // CONSUMER - expected fail
    message = zmsg_new ();
    assert (message);
    zmsg_addstr (message, "CONSUMER");
    // missing stream here
    // missing pattern here
    rv = actor_commands (client, &message);
    assert (rv == 0);
    assert (message == NULL);

    STDERR_NON_EMPTY

    // --------------------------------------------------------------
    fp = freopen (str_stderr_txt.c_str(), "w+", stderr);
    // PRODUCER - expected fail
    message = zmsg_new ();
    assert (message);
    zmsg_addstr (message, "PRODUCER");
    // missing stream here
    rv = actor_commands (client, &message);
    assert (rv == 0);
    assert (message == NULL);

    STDERR_NON_EMPTY

    zmsg_destroy (&message);
    mlm_client_destroy (&client);
    zactor_destroy (&malamute);
    remove (str_stderr_txt.c_str());

    //  @end
    printf ("OK\n");
}
