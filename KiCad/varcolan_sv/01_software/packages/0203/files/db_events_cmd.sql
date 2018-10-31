ALTER TABLE history RENAME TO sqlitestudio_temp_table;

CREATE TABLE history (timestamp DATETIME NOT NULL, terminal_id INTEGER NOT NULL, event INTEGER NOT NULL, badge_id INTEGER, area INTEGER, causal_code INTEGER, user_first_name TEXT (32), user_second_name TEXT (32));

INSERT INTO history (timestamp, terminal_id, event, badge_id, area, causal_code, user_first_name, user_second_name) SELECT timestamp, terminal_id, event, badge_id, area, code, user_first_name, user_second_name FROM sqlitestudio_temp_table;

DROP TABLE sqlitestudio_temp_table;

