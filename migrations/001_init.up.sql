CREATE TABLE chats (
    id         INTEGER PRIMARY KEY,
    username   TEXT,
    first_name TEXT,
    last_name  TEXT
);

CREATE TABLE emails (
    id       INTEGER PRIMARY KEY AUTOINCREMENT,
    chat_id  INTEGER NOT NULL,
    address  TEXT    NOT NULL,
    password TEXT,
    last_uid INTEGER,

    FOREIGN KEY (chat_id) REFERENCES chats(id)
);
