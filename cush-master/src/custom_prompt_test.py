#!/usr/bin/python
#
# Tests the functionality of the `custom` built-in
#
import atexit, proc_check, time
from testutils import *
import socket, os

console = setup_tests()

# Ensure that shell prints expected prompt
no_prompt = "Shell did not print expected prompt (%d)"
expect_prompt(no_prompt % 0)

#################################################################
# Test #1:  Parsing the required tokens (hostname, cwd)
#           Check that we can switch to the prompt,
#           and that its details are parsed correctly

# Get details
custpro = "host[%s] in: %s>" % (socket.gethostname(), os.getcwd())

# To the custom shell
sendline("custom")
expect_exact(custpro, "can't switch to the custom prompt")

#################################################################
# Test #2:  Executing commands
#           Check that the new prompt does not break functionality

# Execute a command
greeting = "Hello, cush!"
sendline("echo " + greeting)
expect_exact(greeting, "can't execute commands at the custom prompt")

#################################################################
# Test #3:  Resetting the prompt
#           Check that the regular prompt can be restored

# Back to regular shell
sendline("custom")
expect_prompt(no_prompt % 1)

test_success()