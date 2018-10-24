ALTER TABLE badge ADD COLUMN timestamp;
CREATE TABLE custom_queries (query_id INTEGER UNIQUE NOT NULL, name TEXT (256), "query" TEXT);

