--/* QupZillKa https://github.com/dualword/QupZillKa License:GNU GPL v3*/
BEGIN TRANSACTION;

CREATE TABLE IF NOT EXISTS "folder" (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    "name" TEXT NOT NULL UNIQUE
);


CREATE TABLE IF NOT EXISTS "feed" (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    fid INTEGER  REFERENCES folder(id) ON DELETE CASCADE,
    "url" TEXT NOT NULL UNIQUE,
    "xmlurl" TEXT,
    "title"	TEXT,
    "description" TEXT,
    "lastbuild" TEXT,
    "pubdate" TEXT,
    "lm" TEXT,
    "active" INTEGER CHECK (active IN (0, 1)),
    "updated" TEXT,
    "icon" BLOB

);
CREATE INDEX IF NOT EXISTS "idx_feed_url" ON feed (url);
CREATE INDEX IF NOT EXISTS "idx_feed_pd" ON feed (pubDate);
CREATE INDEX IF NOT EXISTS "idx_feed_upd" ON feed (updated);

CREATE TABLE IF NOT EXISTS "item" (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    fid INTEGER  REFERENCES feed(id) ON DELETE CASCADE,
    "url" TEXT NOT NULL,
    "title"	TEXT,
    "text"	TEXT,
    "unread" INTEGER CHECK (unread IN (0, 1)),
    "guid"	TEXT,
    "enc_url" TEXT,
    "enc_length" TEXT,
    "enc_type" TEXT,
    "m_url" TEXT,
    "m_type" TEXT,
    "m_thurl" TEXT,
    pubdate TEXT,
    UNIQUE (fid, url)
);
CREATE INDEX IF NOT EXISTS "idx_item_url" ON item (url);
CREATE INDEX IF NOT EXISTS "idx_item_pd" ON item (pubDate);

COMMIT;
