ALTER TABLE terminal RENAME TO sqlitestudio_temp_table;

CREATE TABLE terminal (id INTEGER PRIMARY KEY UNIQUE NOT NULL, name STRING (32) NOT NULL, antipassback INTEGER NOT NULL DEFAULT (0), access1 INTEGER, access2 INTEGER, weektime_id INTEGER, open_door_time INTEGER, open_door_timeout INTEGER, area1_reader1 INTEGER, area1_reader2 INTEGER, area2_reader1 INTEGER, area2_reader2 INTEGER, entrance_type INTEGER, MAC_addr STRING (20), IP_addr STRING (16), filtro_out INTEGER DEFAULT (0), status INTEGER, fw_ver TEXT (16), hw_ver INTEGER);

INSERT INTO terminal (id, name, antipassback, access1, weektime_id, open_door_time, open_door_timeout, area1_reader1, area1_reader2, entrance_type, MAC_addr, IP_addr, filtro_out, status, fw_ver) SELECT id, name, antipassback, access, weektime_id, open_door_time, open_door_timeout, area_in, area_out, entrance_type, MAC_addr, IP_addr, filtro_out, status, fw_ver FROM sqlitestudio_temp_table;

DROP TABLE sqlitestudio_temp_table;

CREATE TABLE causal_codes (n INTEGER, causal_id INTEGER DEFAULT (0), description STRING (20));
