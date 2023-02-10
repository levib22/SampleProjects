import sys
import argparse
import copy
import time



""" 
Look up Tables: taken from https://nvlpubs.nist.gov/nistpubs/FIPS/NIST.FIPS.197.pdf
rcon was precomputed and put into hex values
"""

sbox = (
        0x63, 0x7C, 0x77, 0x7B, 0xF2, 0x6B, 0x6F, 0xC5, 0x30, 0x01, 0x67, 0x2B, 0xFE, 0xD7, 0xAB, 0x76,
        0xCA, 0x82, 0xC9, 0x7D, 0xFA, 0x59, 0x47, 0xF0, 0xAD, 0xD4, 0xA2, 0xAF, 0x9C, 0xA4, 0x72, 0xC0,
        0xB7, 0xFD, 0x93, 0x26, 0x36, 0x3F, 0xF7, 0xCC, 0x34, 0xA5, 0xE5, 0xF1, 0x71, 0xD8, 0x31, 0x15,
        0x04, 0xC7, 0x23, 0xC3, 0x18, 0x96, 0x05, 0x9A, 0x07, 0x12, 0x80, 0xE2, 0xEB, 0x27, 0xB2, 0x75,
        0x09, 0x83, 0x2C, 0x1A, 0x1B, 0x6E, 0x5A, 0xA0, 0x52, 0x3B, 0xD6, 0xB3, 0x29, 0xE3, 0x2F, 0x84,
        0x53, 0xD1, 0x00, 0xED, 0x20, 0xFC, 0xB1, 0x5B, 0x6A, 0xCB, 0xBE, 0x39, 0x4A, 0x4C, 0x58, 0xCF,
        0xD0, 0xEF, 0xAA, 0xFB, 0x43, 0x4D, 0x33, 0x85, 0x45, 0xF9, 0x02, 0x7F, 0x50, 0x3C, 0x9F, 0xA8,
        0x51, 0xA3, 0x40, 0x8F, 0x92, 0x9D, 0x38, 0xF5, 0xBC, 0xB6, 0xDA, 0x21, 0x10, 0xFF, 0xF3, 0xD2,
        0xCD, 0x0C, 0x13, 0xEC, 0x5F, 0x97, 0x44, 0x17, 0xC4, 0xA7, 0x7E, 0x3D, 0x64, 0x5D, 0x19, 0x73,
        0x60, 0x81, 0x4F, 0xDC, 0x22, 0x2A, 0x90, 0x88, 0x46, 0xEE, 0xB8, 0x14, 0xDE, 0x5E, 0x0B, 0xDB,
        0xE0, 0x32, 0x3A, 0x0A, 0x49, 0x06, 0x24, 0x5C, 0xC2, 0xD3, 0xAC, 0x62, 0x91, 0x95, 0xE4, 0x79,
        0xE7, 0xC8, 0x37, 0x6D, 0x8D, 0xD5, 0x4E, 0xA9, 0x6C, 0x56, 0xF4, 0xEA, 0x65, 0x7A, 0xAE, 0x08,
        0xBA, 0x78, 0x25, 0x2E, 0x1C, 0xA6, 0xB4, 0xC6, 0xE8, 0xDD, 0x74, 0x1F, 0x4B, 0xBD, 0x8B, 0x8A,
        0x70, 0x3E, 0xB5, 0x66, 0x48, 0x03, 0xF6, 0x0E, 0x61, 0x35, 0x57, 0xB9, 0x86, 0xC1, 0x1D, 0x9E,
        0xE1, 0xF8, 0x98, 0x11, 0x69, 0xD9, 0x8E, 0x94, 0x9B, 0x1E, 0x87, 0xE9, 0xCE, 0x55, 0x28, 0xDF,
        0x8C, 0xA1, 0x89, 0x0D, 0xBF, 0xE6, 0x42, 0x68, 0x41, 0x99, 0x2D, 0x0F, 0xB0, 0x54, 0xBB, 0x16
        )

sbox_inverse = [
        0x52, 0x09, 0x6A, 0xD5, 0x30, 0x36, 0xA5, 0x38, 0xBF, 0x40, 0xA3, 0x9E, 0x81, 0xF3, 0xD7, 0xFB,
        0x7C, 0xE3, 0x39, 0x82, 0x9B, 0x2F, 0xFF, 0x87, 0x34, 0x8E, 0x43, 0x44, 0xC4, 0xDE, 0xE9, 0xCB,
        0x54, 0x7B, 0x94, 0x32, 0xA6, 0xC2, 0x23, 0x3D, 0xEE, 0x4C, 0x95, 0x0B, 0x42, 0xFA, 0xC3, 0x4E,
        0x08, 0x2E, 0xA1, 0x66, 0x28, 0xD9, 0x24, 0xB2, 0x76, 0x5B, 0xA2, 0x49, 0x6D, 0x8B, 0xD1, 0x25,
        0x72, 0xF8, 0xF6, 0x64, 0x86, 0x68, 0x98, 0x16, 0xD4, 0xA4, 0x5C, 0xCC, 0x5D, 0x65, 0xB6, 0x92,
        0x6C, 0x70, 0x48, 0x50, 0xFD, 0xED, 0xB9, 0xDA, 0x5E, 0x15, 0x46, 0x57, 0xA7, 0x8D, 0x9D, 0x84,
        0x90, 0xD8, 0xAB, 0x00, 0x8C, 0xBC, 0xD3, 0x0A, 0xF7, 0xE4, 0x58, 0x05, 0xB8, 0xB3, 0x45, 0x06,
        0xD0, 0x2C, 0x1E, 0x8F, 0xCA, 0x3F, 0x0F, 0x02, 0xC1, 0xAF, 0xBD, 0x03, 0x01, 0x13, 0x8A, 0x6B,
        0x3A, 0x91, 0x11, 0x41, 0x4F, 0x67, 0xDC, 0xEA, 0x97, 0xF2, 0xCF, 0xCE, 0xF0, 0xB4, 0xE6, 0x73,
        0x96, 0xAC, 0x74, 0x22, 0xE7, 0xAD, 0x35, 0x85, 0xE2, 0xF9, 0x37, 0xE8, 0x1C, 0x75, 0xDF, 0x6E,
        0x47, 0xF1, 0x1A, 0x71, 0x1D, 0x29, 0xC5, 0x89, 0x6F, 0xB7, 0x62, 0x0E, 0xAA, 0x18, 0xBE, 0x1B,
        0xFC, 0x56, 0x3E, 0x4B, 0xC6, 0xD2, 0x79, 0x20, 0x9A, 0xDB, 0xC0, 0xFE, 0x78, 0xCD, 0x5A, 0xF4,
        0x1F, 0xDD, 0xA8, 0x33, 0x88, 0x07, 0xC7, 0x31, 0xB1, 0x12, 0x10, 0x59, 0x27, 0x80, 0xEC, 0x5F,
        0x60, 0x51, 0x7F, 0xA9, 0x19, 0xB5, 0x4A, 0x0D, 0x2D, 0xE5, 0x7A, 0x9F, 0x93, 0xC9, 0x9C, 0xEF,
        0xA0, 0xE0, 0x3B, 0x4D, 0xAE, 0x2A, 0xF5, 0xB0, 0xC8, 0xEB, 0xBB, 0x3C, 0x83, 0x53, 0x99, 0x61,
        0x17, 0x2B, 0x04, 0x7E, 0xBA, 0x77, 0xD6, 0x26, 0xE1, 0x69, 0x14, 0x63, 0x55, 0x21, 0x0C, 0x7D
        ]

rcon = [
        0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x1b, 0x36, 0x6c, 0xd8, 0xab, 0x4d, 0x9a
        ]

def parse_args():
    """ This function handles the command line interface
    return: 
        the arguments from the command line
    """
    parser = argparse.ArgumentParser()
    parser.add_argument('-p', '--plaintext',
                        help='plaintext or cipher text message to be (de)encrypted, required to run program, based on spefification', 
                        required=True)
    parser.add_argument('-k', '--key',
                        help='Cryptographic Key required to run AES, required to run program',
                        required=True)              
    parser.add_argument('-d', '--debug',
                        help='debug mode, will print out different things that will hopfully help with debugging the program',
                        action='store_true')
    parser.add_argument('-cbc', '--cbc-encryption',
                        help='Turning on cbc mode on or off, default off',
                        action='store_true')
    parser.add_argument('-e', '--encryption',
                        help='Encrypt this text',
                        action='store_true') 
    parser.add_argument('-de', '--decryption',
                        help='DEncrypt this text',
                        action='store_true') 
    parser.add_argument('-f', '--filestore',
                        help='Do you want to store the result in a file? use the -f flag',
                        action='store_true') 



    args = parser.parse_args()

    return args       


def to_matrix(input, args):
    """ This breaks up the plaintext, or chipher text recived and turns it into blocks that can be encypted 
        using AES
    Args:
        input: The plaintext, in a hex string, must be at least 32 characters long to form one
        block, or some multiple of 32 for multiple blocks
        args: the arguments from the command line
    return: 
        A list of plaintext blocks to be encypted by AES
    """

    # Making sure the size is at least one block long since there was no padding implemented
    if len(input) < 32:
        print("Enter Valid Input length (or I might have to change this later )")


    block_arr = []
    blocks = len(input) // 32

    # splitting up the blocks
    for i in range(blocks):
        ans = [[0] * 4, [0] * 4, [0] * 4, [0] * 4]
        r, c = 0,0

        #start and end 
        s, e = (len(input) // blocks * i), ((len(input) // blocks * i) + len(input) // blocks)
        for i in range(s, e, 2):
            ans[r][c] = input[i:i+2]
            if r == 3:
                c += 1
                r = 0
            else:
                r += 1

        ans_hex = [list(map(lambda x:int(x, 16), group)) for group in ans]
        block_arr.extend([ans_hex])
    
    
    if args.debug: 
        print(ans)
        print(ans_hex)
    return block_arr


def shift_rows(matrix):
    """ This function takes a list of list (matrix) and shifts them
        in the way from AES spec.  
        
        1st row -> no shift
        2nd row -> 1 left shift
        3rd row -> 2 left shift
        4th row -> 3 left shift
    Args:
        matrix: a list of lists that is in the form of a square matix
    return:
        The shifted matrix 
    """


    for i in range(len(matrix)):
        matrix[i] = matrix[i][i:] + matrix[i][:i]
    return matrix

def inv_shift_rows(matrix):
    """ This function takes a list of list (matrix) and shifts them
        in the way from AES spec in the oppoiste way.  
        
        1st row -> no shift
        2nd row -> 1 right shift
        3rd row -> 2 right shift
        4th row -> 3 right shift
    Args:
        matrix: a list of lists that is in the form of a square matix
    return:
        The shifted matrix 
    """


    for i in range(len(matrix)):
        matrix[i] = matrix[i][-i:] + matrix[i][:-i]
    return matrix

def key_expansion_core(byte_arr, round, args):
    """ Takes a four byte string (byte_arr) and preforms three steps
        1. Rotate Left
        2. S-box
        3. Round Constant
        
        And uses it later in the main key_expansion() function
    Args:
        byte_arr: a four byte string
        round: what round it is to get the reference to the round constant
        or rcon
        args: the arguments from the command line
    return: 
        the changed byte for the new string
    """
    if args.debug:
        print(byte_arr)

    # Rotate Left
    temp = byte_arr[0]
    byte_arr[0] = byte_arr[1]
    byte_arr[1] = byte_arr[2]
    byte_arr[2] = byte_arr[3]
    byte_arr[3] = temp

    if args.debug:
        print(byte_arr)

    # S-box
    byte_arr = [sbox[x] for x in byte_arr]

    if args.debug:
        print(byte_arr)

    # Round Constant
    byte_arr[0] ^= rcon[round]

    if args.debug:
        print(byte_arr)

    return byte_arr


def key_expansion(initial_key, args): 
    """ Expands the inital key from either 16, 24, 32 bytes to 176, 208, 240 bytes respectivly.
        You will keep appended the key to the end of the inital key
    Args:
        initial_key: The original 
        args: the arguments from the command line
    return: 
        The expanded key
    """
    # turning key into array of hex nibbles
    temp = [initial_key[i:i+2] for i in range(0, len(initial_key), 2)]
    expanded_key = [int(x, 16) for x in temp]

    # vars for different types of key lengths
    if len(expanded_key) == 16: 
        final_len = 176
        j_len = 4
        get_bytes = -16
    elif len(expanded_key) == 24: 
        final_len = 208
        j_len = 6
        get_bytes = -24
    else: 
        final_len = 240
        j_len = 8
        get_bytes = -32
    
    if args.debug:
        print(expanded_key)
        print("Expanstion key Length:", len(expanded_key))

    round = 0
    while len(expanded_key) < final_len: 
        for j in range(j_len): 
            temp1 = expanded_key[-4:]
            if j == 0: 
                temp1 = key_expansion_core(temp1, round, args)
            if j == 4 and final_len == 240: 
                temp1 = [sbox[x] for x in temp1]
            temp2 = expanded_key[get_bytes:]
            temp3 = temp2[0:4]
            tempf = [temp1[i] ^ temp3[i] for i in range(len(temp1))]
            expanded_key.extend(tempf)
        round += 1
    if args.debug:
        hexkey = [format(x, 'x') for x in expanded_key]
        print(hexkey)

    return expanded_key
    
    
def add_round_key(block, round_key, args): 
    """ XORs the round key into the current block of ciphertext (or plaintext for the first round)
    Args:
        block: the current 16 bytes we are encrypting 
        round_key: the current round key we are XORing the block with
        args: the arguments from the command line
    return: 
        returns the xor'ed block
    """

    # converting the key into a matrix form
    key = [round_key[k::4] for k in range(4)]


    for i in range(len(block)):
        for j in range(len(block)): 
            block[i][j] ^= key[i][j]

    return block


def sub_bytes(block, args):
    """ This function subsitudes every byte in a block with the sbox values
    Args:
        state: the current block we are looking at
        args: the arguments from the command line
    return: 
        The subistuded block
    """
    
    block = [list(map(lambda x:sbox[x], group)) for group in block]
    return block

def inv_sub_bytes(block, args):
    """ This function subsitudes every byte in a block with the sbox values
    Args:
        state: the current block we are looking at
        args: the arguments from the command line
    return: 
        The subistuded block
    """
    
    block = [list(map(lambda x:sbox_inverse[x], group)) for group in block]
    return block


def mix_column(column, args):
    """ This is the hard method of the encryption step, this function will preform
        matrix multiplication over the feild of AES. 100011011. Both this function
        and the inverse function I found easier and more understanable (making it
        esier to debug when things started to go wrong) to implement
        a multiplication function (finite_feild_mult) and then XOR those values rather
        than trying to multiple with bitwise here. It is not the most efficient way
        to do this but it works. 
    Args:
        column: the column of bytes we want to mix
        args: the arguments from the command line
    return: 
        the result from the matrix multipication over the feild
    """

    # using a temp variable since all updates are not happening simotaniously it messed up the blocks during testing
    temp = [0] * 4

    temp[0] = finite_feild_mult(2, column[0]) ^ finite_feild_mult(3, column[1]) ^ finite_feild_mult(1, column[2]) ^ finite_feild_mult(1, column[3])
    temp[1] = finite_feild_mult(1, column[0]) ^ finite_feild_mult(2, column[1]) ^ finite_feild_mult(3, column[2]) ^ finite_feild_mult(1, column[3])
    temp[2] = finite_feild_mult(1, column[0]) ^ finite_feild_mult(1, column[1]) ^ finite_feild_mult(2, column[2]) ^ finite_feild_mult(3, column[3])
    temp[3] = finite_feild_mult(3, column[0]) ^ finite_feild_mult(1, column[1]) ^ finite_feild_mult(1, column[2]) ^ finite_feild_mult(2, column[3])

    if args.debug: 
        hexkey = [format(x, 'x').zfill(2) for x in temp]
        print(hexkey)

    return temp

def inverse_mix_column(column, args):
    """ This is the hard method of the encryption step, this function will preform
        matrix multiplication over the feild of AES. 100011011. 
    Args:
        column: the column of bytes we want to mix
        args: the arguments from the command line
    return: 
        the result from the matrix multipication over the feild
    """
    temp = [0] * 4

    if args.debug: 
        hexkey = [format(x, 'x').zfill(2) for x in column]
        print(hexkey)

    temp[0] = finite_feild_mult(14, column[0]) ^ finite_feild_mult(11, column[1]) ^ finite_feild_mult(13, column[2]) ^ finite_feild_mult(9, column[3])
    temp[1] = finite_feild_mult(9, column[0]) ^ finite_feild_mult(14, column[1]) ^ finite_feild_mult(11, column[2]) ^ finite_feild_mult(13, column[3])
    temp[2] = finite_feild_mult(13, column[0]) ^ finite_feild_mult(9, column[1]) ^ finite_feild_mult(14, column[2]) ^ finite_feild_mult(11, column[3])
    temp[3] = finite_feild_mult(11, column[0]) ^ finite_feild_mult(13, column[1]) ^ finite_feild_mult(9, column[2]) ^ finite_feild_mult(14, column[3])

    if args.debug: 
        hexkey = [format(x, 'x').zfill(2) for x in temp]
        print(hexkey)

    return temp

def encrypt(block, expanded_key, round_num, block_arr, args):
    """ This method encrypts the plaintext following the AES standard
    Args:
        block: The current block of plaintext we are encrypting
         expanded_key: the currenlty expanded key
         round_num: the current round of encryption (in terms of how many blocks we need to do)
                    keeping track of this for the CBC mode
            
         block_arr: the already encypted blocks (if any have been encrypted)
         args: the arguments from the command line
    return: 
        A block of encrypted ciphertext
    """

    keylen,rounds = None, None 

    # setting why type of encryptions (128, 192, 256 bit versions)
    if len([args.key[i:i+2] for i in range(0, len(args.key), 2)]) == 16:
        rounds = 10 
        keylen = 16
    elif len([args.key[i:i+2] for i in range(0, len(args.key), 2)]) == 24: 
        rounds = 12
        keylen = 16
    else: 
        rounds = 14 
        keylen = 16


    # Initial add round step
    block = add_round_key(block, expanded_key[0:keylen], args)

    # If cbc flag is turned on then xor the last chiperblock into the current block
    if round_num != 0 and args.cbc_encryption:
        print("cbc needed")
        tempblock = block_arr[-1]
        for i in range(len(block)):
            for j in range(len(block)): 
                block[i][j] ^= tempblock[i][j]

    for i in range(1,rounds): 
        b = i * keylen
        c = i * keylen + keylen

        block = sub_bytes(block, args) # subing the bytes out with the sbox

        block = shift_rows(block) # shifting the rows

        # Need to change the way the matrix stores the columns into rows for the
        # way I implemented this
        temp_state = list(map(list, zip(*block)))
        for j in range(4):
            temp_state[j] = mix_column(temp_state[j], args)
        
        # chaning the columns from rows again
        block = list(map(list, zip(*temp_state)))

        block = add_round_key(block, expanded_key[b:c], args) # adding round key into block

    # start and end indicies for matrix
    b = rounds*keylen
    c = (rounds + 1)*keylen

    # last round without mix columns
    block = sub_bytes(block, args)
    block = shift_rows(block)
    block = add_round_key(block, expanded_key[b:c], args)
    
    return block


def decrypt(chipherblock, expanded_key, round_num, block_arr, args):
    """ This method decrpyts the chipertext following the AES standard
    Args:
        chipherblock: The current block of plaintext we are encrypting
         expanded_key: the currenlty expanded key
         round_num: the current round of encryption (in terms of how many blocks we need to do)
                    keeping track of this for the CBC mode
            
         block_arr: the already encypted blocks (if any have been encrypted)
         args: the arguments from the command line
    return: 
        A block of encrypted ciphertext
    """

    keylen,rounds = None, None 

    # setting why type of encryptions (128, 192, 256 bit versions)
    if len([args.key[i:i+2] for i in range(0, len(args.key), 2)]) == 16:
        rounds = 10 
        keylen = 16
    elif len([args.key[i:i+2] for i in range(0, len(args.key), 2)]) == 24: 
        rounds = 12
        keylen = 16
    else: 
        rounds = 14 
        keylen = 16

    if args.debug: 
        print("Orginial Cipher Text ") 
        concat_blocks([chipherblock], args)
        print()

    start = rounds*keylen
    end = (rounds + 1)*keylen
    rounds -= 1

    chipherblock = add_round_key(chipherblock, expanded_key[start:end], args) # adding the last round key

    if args.debug: 
        print("First Add Round Key") 
        concat_blocks([chipherblock], args)
        print()


    chipherblock = inv_shift_rows(chipherblock) # inverse shift rows 

    if args.debug: 
        print("Inverse Shift rows")
        concat_blocks([chipherblock], args)
        print()
    
    chipherblock = inv_sub_bytes(chipherblock, args) # inverse sub bytes 

    if args.debug: 
        print("Inverse Sub Bytes")
        concat_blocks([chipherblock], args)
        print()

    # we need to have these three outside the loop in order to mirror image the encryption

    while rounds > 0: 
        start = rounds*keylen
        end = (rounds + 1)*keylen

        # mirror image of encryption loop
        chipherblock = add_round_key(chipherblock, expanded_key[start:end], args)

        if args.debug: 
            print("Add round Key")
            concat_blocks([chipherblock], args)
            print()

        temp_state = list(map(list, zip(*chipherblock)))
        for i in range(4):
            temp_state[i] = inverse_mix_column(temp_state[i], args)
        chipherblock = list(map(list, zip(*temp_state)))

        if args.debug: 
            print("Inverse Mix columns")
            concat_blocks([chipherblock], args)
            print()

        chipherblock = inv_shift_rows(chipherblock)

        if args.debug: 
            print("Inverse Shift rows")
            concat_blocks([chipherblock], args)
            print()

        chipherblock = inv_sub_bytes(chipherblock, args)
        if args.debug: 
            print("Inverse Sub Bytes")
            concat_blocks([chipherblock], args)
            print()

        rounds -= 1

    # cbc inverse 
    if round_num != 0 and args.cbc_encryption:
        print("cbc needed")
        tempblock = block_arr[(round_num - 1)]
        for i in range(len(chipherblock)):
            for j in range(len(chipherblock)): 
                chipherblock[i][j] ^= tempblock[i][j]

    chipherblock = add_round_key(chipherblock, expanded_key[0:keylen], args)



    if args.debug: 
        print("Final Add round key")
        concat_blocks([chipherblock], args)
        print()


    return chipherblock



def finite_feild_mult(x, y):
    """ Multiplies two numbers in gf2^8
    Args:
        input: 
    return: 
        a number in the feild space
    """
    temp, high = 0, 0
    while y:
        if y & 1:
            temp ^= x
        high = x & 128 # checking if the high bit is set (x^8)
        x <<= 1 
        y >>= 1
        if high:
            x ^= 27 # already shifted out the leading one so 27 is the 
            # irreducable polynomial without the leading one
    return temp % 256


def concat_blocks(blocks, args): 
    """ returning the hex string of a block
    Args:
        blocks: an array of blocks of text
        args: the arguments from the command line
    return: 
            A formatted hex string
    """

    if args.debug: 
        hexkey = [[format(x, 'x').zfill(2) for x in group] for group in blocks[0]]
        print(hexkey)

    outputstr = ''
    for block in blocks: 
        hexkey = [[format(x, 'x').zfill(2) for x in group] for group in block]
        temphex = list(map(list, zip(*hexkey)))
        temp = ''.join(str(r) for v in temphex for r in v)
        outputstr += temp
    print(outputstr)
    return outputstr


# main method of the file


if __name__ == "__main__":

    args = parse_args()
    print("Starting Ecryption on: ", args.plaintext, " with key:", args.key)
    
    if args.debug: 
        print("Debug mode ON")

    if(args.cbc_encryption): 
        print("Using CBC mode")


    if(args.encryption): 
        states = to_matrix(args.plaintext, args)
        expanded_key = key_expansion(args.key, args)
        hexkey = [format(x, 'x') for x in expanded_key]
        block_arr = []
        for x in range(len(states)): 
            block_arr.append(encrypt(states[x], expanded_key, x, block_arr, args))

        final = concat_blocks(block_arr, args)

        if (args.filestore):
            filename = "encrypt" + str(int(time.time())) + ".txt"
            f= open(filename,"w+")
            f.write(final)
            f.close() 

    elif(args.decryption): 
        chipertext_arr = to_matrix(args.plaintext, args)
        block_arr = copy.deepcopy(chipertext_arr)
        expanded_key = key_expansion(args.key, args)
        deblock_arr = []
        for x in range(len(chipertext_arr)):
            deblock_arr.append(decrypt(chipertext_arr[x], expanded_key, x, block_arr, args))
        final = concat_blocks(deblock_arr, args)

        if (args.filestore):
            filename = "decrypt" + str(int(time.time())) + ".txt"
            f= open(filename,"w+")
            f.write(final)
            f.close() 
    else:
        print("need to select a mode either -e or -de")