import sqlite3
import argparse


def add_user(name, token):
    """
    Adds a new user to the User table in the database.

    Args:
        name (str): Username to add to the database
        token (str): User's authentication token

    Returns:
        None
    """
    # Connect to the SQLite database
    try:
        conn = sqlite3.connect("ghost.db")
        cursor = conn.cursor()

        # Insert the new user into the User table
        cursor.execute('''
            INSERT INTO User (ID, token)
            VALUES (?, ?)
        ''', (name, token))

        # Commit the transaction
        conn.commit()
        print(f"User '{name}' added successfully with token: {token}")

    except sqlite3.IntegrityError as e:
        print(f"Error: {e}. User '{name}' may already exist.")
    except sqlite3.OperationalError as e:
        print(f"Database error: {e}. Make sure 'ghost.db' exists in the same directory as this script.")
    finally:
        # Close the connection if it was opened
        if 'conn' in locals():
            conn.close()


def main():
    # Set up argument parser
    parser = argparse.ArgumentParser(
        description="Add a new user to the Ghost streaming service database.",
        epilog="Note: The ghost.db database file must exist in the same directory as this script."
    )

    # Add required arguments
    parser.add_argument('--user', required=True, help="Username to add to the database")
    parser.add_argument('--token', required=True, help="Authentication token for the user")

    # Parse arguments
    args = parser.parse_args()

    # Add the user
    add_user(args.user, args.token)


if __name__ == "__main__":
    main()