--/* QupZillKa https://github.com/dualword/QupZillKa License:GNU GPL v3*/
BEGIN TRANSACTION;
CREATE TABLE IF NOT EXISTS "autofill_exceptions" (
	"id"	INTEGER,
	"server"	TEXT,
	PRIMARY KEY("id")
);
CREATE TABLE IF NOT EXISTS "autofill" (
	"data"	TEXT,
	"id"	INTEGER,
	"password"	TEXT,
	"server"	TEXT,
	"username"	TEXT,
	"last_used"	NUMERIC,
	PRIMARY KEY("id")
);
CREATE TABLE IF NOT EXISTS "history" (
	"title"	VARCHAR(200),
	"count"	NUMERIC,
	"id"	INTEGER,
	"date"	NUMERIC,
	"url"	VARCHAR(256),
	PRIMARY KEY("id")
);
CREATE TABLE IF NOT EXISTS "search_engines" (
	"id"	INTEGER,
	"name"	TEXT,
	"icon"	TEXT,
	"url"	TEXT,
	"shortcut"	TEXT,
	"suggestionsUrl"	TEXT,
	"suggestionsParameters"	TEXT,
	"postData"	TEXT,
	PRIMARY KEY("id")
);
CREATE TABLE IF NOT EXISTS "autofill_encrypted" (
	"data_encrypted"	TEXT,
	"id"	INTEGER,
	"password_encrypted"	TEXT,
	"server"	TEXT,
	"username_encrypted"	TEXT,
	"last_used"	NUMERIC,
	PRIMARY KEY("id")
);
CREATE TABLE IF NOT EXISTS "icons" (
	"icon"	BLOB,
	"id"	INTEGER,
	"url"	TEXT,
	PRIMARY KEY("id")
);
CREATE TABLE IF NOT EXISTS "rss" (
	"icon"	TEXT,
	"address"	TEXT,
	"id"	INTEGER,
	"title"	TEXT,
	PRIMARY KEY("id")
);
CREATE INDEX IF NOT EXISTS "autofillServer" ON "autofill" (
	"server"	ASC
);
CREATE INDEX IF NOT EXISTS "autofillExceptionServer" ON "autofill_exceptions" (
	"server"	ASC
);
CREATE INDEX IF NOT EXISTS "historyTitle" ON "history" (
	"title"	ASC
);
CREATE INDEX IF NOT EXISTS "autofillEncryptedServer" ON "autofill_encrypted" (
	"server"	ASC
);
CREATE UNIQUE INDEX IF NOT EXISTS "historyUrl" ON "history" (
	"url"	ASC
);
CREATE UNIQUE INDEX IF NOT EXISTS "iconsUrl" ON "icons" (
	"url"	ASC
);
COMMIT;
