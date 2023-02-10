#!/usr/bin/python
#
# Tests the functionality of the `history` built-in
#
import atexit, proc_check, time
from testutils import *

console = setup_tests()

# For generating random strings
import string, random
random.seed(42)

# Ensure that shell prints expected prompt
no_prompt = "Shell did not print expected prompt (%d)"
expect_prompt(no_prompt % 0)

# Test configurations
N = 10
M = 10
history_regex = "%5d %s"
history_offset = 1

#################################################################
# Test #1: Adding commands and printing the full history

# Generate a list of random strings
letters = string.ascii_lowercase
text = ["".join(random.choice(letters) \
    for j in range(M)) for i in range(N)]

# Send the strings as commands
for word in text:
    sendline("echo " + word)
    expect_exact(word, "echo does not seem to work")
    expect_prompt(no_prompt % 1)
current = N + history_offset
sendline("history")

# Validate that the output matches
echo = "".join((history_regex + "\r\n") \
    % (i + history_offset, "echo " + word) \
    for i, word in enumerate(text))
history = history_regex % (current, "history")
expect_exact(echo + history, "full history is not printed")
expect_prompt(no_prompt % 2)

#################################################################
# Test #2: Printing a truncated history

# Query for everything after the `echo` commands
current += 1
command = "history %d" % (current + 1 - (N + history_offset))
command = "echo START; " + command
sendline(command)

# Validate that the correct amount is printed
history += "\r\n" + history_regex % (current, command)
expect_exact("START\r\n" + history, "truncated history is not printed")
expect_prompt(no_prompt % 3)

#################################################################
# Test #3: Navigating through the history via arrow keys

# Press a bunch of keys and see where we end up
KEY_UP = '\x1b[A'
KEY_DOWN = '\x1b[B'
current += 1
sendline(KEY_UP + KEY_DOWN + KEY_UP * 2)

# We should have called `history` again
history += "\r\n" + history_regex % (current, "history")
expect_exact(echo + history, "arrow keys do not navigate")
expect_prompt(no_prompt % 4)

#################################################################
# Test #4: Retrieving a previous command by index

sendline("!1")
expect_exact(text[0], "positive history indices")
sendline("!-1")
expect_exact(text[0], "negative history indices")

#################################################################
# Test #5: Retrieving a previous command by name

sendline("!e")
expect_exact(text[0], "negative history indices")

test_success()