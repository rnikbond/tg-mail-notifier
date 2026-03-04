CREATE TABLE chats (
    id         INTEGER PRIMARY KEY,
    username   TEXT,
    first_name TEXT,
    last_name  TEXT
);

CREATE TABLE emails (
    id           INTEGER PRIMARY KEY AUTOINCREMENT,
    chat_id      INTEGER NOT NULL,
    address      TEXT    NOT NULL,
    last_uid     INTEGER,
    password     BLOB,
    password_iv  BLOB,
    password_tag BLOB,

    FOREIGN KEY (chat_id) REFERENCES chats(id)
);
