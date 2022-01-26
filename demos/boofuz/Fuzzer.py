#!/usr/bin/python

from subprocess import call
from boofuzz import * 


# Checking for fails
def get_end(target, fuzz_data_logger, session, sock, *args, **kwargs):

    end_message_template = b"End"
    try:
        end_message = target.recv(1024)
    except:
        #print("Unable to connect. Target is down. Exiting.")
        exit(1)

    fuzz_data_logger.log_check('Receiving end...')
    if end_message_template in end_message:
        fuzz_data_logger.log_pass('End received')
    else:
        fuzz_data_logger.log_fail('No end received')
        #print("No end received...")


def define_proto(session):
    # disable Black formatting to keep custom indentation
    # fmt: off
    user = Request("user", children=(
        String(name="key", default_value="USER", max_len = 100),
        Static(name="end", default_value="\n"),
    ))

    file_read = Request("file_read", children=(
        String(name="key", default_value="USER", max_len = 24),
        Static(name="endUser", default_value="\n"),
        Delim(name="command", default_value="r|"),
        String(name="file", default_value="FILE"),
        Static(name="endFile", default_value="\n"),
    ))
    file_write = Request("file_write", children=(
        String(name="key", default_value="USER", max_len = 24),
        Static(name="endUser", default_value="\n"),
        Delim(name="command", default_value="w|"),
        String(name="file", default_value="FILE"),
        Static(name="endFile", default_value="\n"),
        String(name="fileContents", default_value="FileContents", max_len = 512)
    ))

    session.connect(file_read)

    #session.connect(user)
    #session.connect(file_read, callback=get_end)
    #session.connect(file_write, callback=get_end)

def main():


    session = Session(target=Target(connection = SocketConnection("localhost", 40000, proto='tcp')), post_test_case_callbacks=[get_end])

    define_proto(session)

    session.fuzz()



if __name__ == "__main__":
    main()