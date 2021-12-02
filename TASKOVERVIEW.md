The entities of the program are:
    > Client
    > Client Message Manager Daemon
    > Server

The Client process will:

    >Open the message queue on which the Client Daemon is
    listening to

    >Send the message or command sent by the user using the
    command /send (...)

    >Close the message queue

    >Exit the process

The Client Message Manager Daemon:

    > Main process:
        >Connect to the message queue that the user will
        communicate with

        >Establish the TCP-IP connection with the server

        >Ask user name

        >Send the user information to the server

        >Run on the background

        >Create two threads: read and write

    > Read Task
        >Receives command from the server

        >Parse the command

        >Run the command:
            > echo: print the message to a log file

            > client: print clients' information to
            a log file

    > Write Task
        >Checks if there are messages left on the message
        queue

        >If there are, process them based on the information:
            >Whether it contains command (/channel or /clients)
            or message

            >If the user is AFK and sent the message, put the status
            as ONLINE

            >Run the command
                > message: send the echo command with the message to
                the server

                > channel: send the channel command with the users to
                the server

                > client: send the client command to the server

The server:
    >The server main process:
        >Open the TCP-IP connection

        >Accept and process clients' information

        >Opens chat thread for each client

    >The chat thread will:
        >Receive commands from the Client's Daemon

        >Process each command and run it:
            > message: echo the message to all the users specified
            in the channel of the user

            > channel: change the users' channel

            > client: return to the Client's Daemon the list of
            clients connected to the server