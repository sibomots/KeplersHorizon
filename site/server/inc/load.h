//////////////////////////////////////////////////////////////////////////////////
// This file is part of Kepler's Horizon
//
// Licensed under BSD 3-Clause License
//
// Copyright (c) 2025, sibomots
/////////////////////////////////////////////////////////////////////////////////
#ifndef __LOAD_H__
#define __LOAD_H__

#include <vector>
#include <string>

// Tables to drop (in reverse dependency order for --clean)
static const std::vector<std::string> DROP_STATEMENTS = {
    "DROP TABLE IF EXISTS telemetry_queue",
    "DROP TABLE IF EXISTS combat_orders",
    "DROP TABLE IF EXISTS combat_state",
    "DROP TABLE IF EXISTS warpline_hexes",
    "DROP TABLE IF EXISTS hexes",
    "DROP TABLE IF EXISTS warplines",
    "DROP TABLE IF EXISTS star_systems",
    "DROP TABLE IF EXISTS sightings",
    "DROP TABLE IF EXISTS ships",
    "DROP TABLE IF EXISTS drafts",
    "DROP TABLE IF EXISTS game_events",
    "DROP TABLE IF EXISTS game_seats",
    "DROP TABLE IF EXISTS games",
    "DROP TABLE IF EXISTS sessions",
    "DROP TABLE IF EXISTS rooms",
    "DROP TABLE IF EXISTS users"
};

// Schema CREATE statements (in dependency order for --schema)
static const std::vector<std::string> SCHEMA_STATEMENTS = {
    "CREATE DATABASE IF NOT EXISTS khdb",
    "USE khdb",
    
    // users
    "CREATE TABLE IF NOT EXISTS users ("
    "id INT AUTO_INCREMENT PRIMARY KEY,"
    "username VARCHAR(64) NOT NULL UNIQUE,"
    "email VARCHAR(255) UNIQUE,"
    "password_plain VARCHAR(128),"
    "password_hash VARCHAR(255),"
    "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP"
    ")",
    
    // Demo users
    "INSERT IGNORE INTO users(username,password_plain) VALUES('alice','alicepw'),('bob','bobpw')",
    
    // rooms
    "CREATE TABLE IF NOT EXISTS rooms ("
    "id INT AUTO_INCREMENT PRIMARY KEY,"
    "room_code VARCHAR(8) NOT NULL UNIQUE,"
    "name VARCHAR(64),"
    "created_by INT NOT NULL,"
    "seat_a INT DEFAULT NULL,"
    "seat_b INT DEFAULT NULL,"
    "game_id INT DEFAULT NULL,"
    "status ENUM('waiting','ready','playing','finished') DEFAULT 'waiting',"
    "scenario VARCHAR(16) DEFAULT NULL,"
    "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,"
    "updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,"
    "FOREIGN KEY (created_by) REFERENCES users(id),"
    "FOREIGN KEY (seat_a) REFERENCES users(id),"
    "FOREIGN KEY (seat_b) REFERENCES users(id),"
    "INDEX (status),"
    "INDEX (room_code)"
    ")",
    
    // sessions
    "CREATE TABLE IF NOT EXISTS sessions ("
    "token CHAR(64) PRIMARY KEY,"
    "user_id INT NOT NULL,"
    "game_id INT DEFAULT NULL,"
    "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,"
    "last_seen TIMESTAMP DEFAULT CURRENT_TIMESTAMP,"
    "FOREIGN KEY (user_id) REFERENCES users(id)"
    ")",
    
    // games
    "CREATE TABLE IF NOT EXISTS games ("
    "id INT AUTO_INCREMENT PRIMARY KEY,"
    "room_id INT DEFAULT NULL,"
    "scenario VARCHAR(16) DEFAULT NULL,"
    "state_json MEDIUMTEXT NOT NULL,"
    "current_draft_A VARCHAR(4) DEFAULT NULL,"
    "current_draft_B VARCHAR(4) DEFAULT NULL,"
    "active_combat_hex VARCHAR(8) DEFAULT NULL,"
    "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,"
    "FOREIGN KEY (room_id) REFERENCES rooms(id)"
    ")",
    
    // game_seats
    "CREATE TABLE IF NOT EXISTS game_seats ("
    "game_id INT NOT NULL,"
    "user_id INT NOT NULL,"
    "seat CHAR(1) NOT NULL,"
    "PRIMARY KEY (game_id, seat),"
    "UNIQUE KEY uniq_user_game (game_id, user_id),"
    "FOREIGN KEY (game_id) REFERENCES games(id),"
    "FOREIGN KEY (user_id) REFERENCES users(id)"
    ")",
    
    // game_events
    "CREATE TABLE IF NOT EXISTS game_events ("
    "id BIGINT AUTO_INCREMENT PRIMARY KEY,"
    "game_id INT NOT NULL,"
    "user_id INT NOT NULL,"
    "seq INT NOT NULL,"
    "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,"
    "command_text VARCHAR(256) NOT NULL,"
    "result_text MEDIUMTEXT NOT NULL,"
    "state_json MEDIUMTEXT NOT NULL,"
    "FOREIGN KEY (game_id) REFERENCES games(id),"
    "FOREIGN KEY (user_id) REFERENCES users(id),"
    "UNIQUE KEY uniq_game_seq (game_id, seq)"
    ")",
    
    // drafts
    "CREATE TABLE IF NOT EXISTS drafts ("
    "id BIGINT AUTO_INCREMENT PRIMARY KEY,"
    "game_id INT NOT NULL,"
    "owner CHAR(1) NOT NULL,"
    "ship_code VARCHAR(4) NOT NULL,"
    "ship_name VARCHAR(32) NOT NULL,"
    "ship_type CHAR(1) NOT NULL,"
    "pd INT NOT NULL DEFAULT 0,"
    "beam INT NOT NULL DEFAULT 0,"
    "screen INT NOT NULL DEFAULT 0,"
    "tube INT NOT NULL DEFAULT 0,"
    "missiles INT NOT NULL DEFAULT 0,"
    "sr INT NOT NULL DEFAULT 0,"
    "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,"
    "UNIQUE KEY uniq_draft (game_id, owner, ship_code),"
    "FOREIGN KEY (game_id) REFERENCES games(id)"
    ")",
    
    // ships
    "CREATE TABLE IF NOT EXISTS ships ("
    "id BIGINT AUTO_INCREMENT PRIMARY KEY,"
    "game_id INT NOT NULL,"
    "owner CHAR(1) NOT NULL,"
    "ship_code VARCHAR(4) NOT NULL,"
    "ship_name VARCHAR(32) NOT NULL,"
    "ship_type CHAR(1) NOT NULL,"
    "tech_level INT NOT NULL DEFAULT 0,"
    "built_turn VARCHAR(8) NOT NULL,"
    "pd INT NOT NULL DEFAULT 0,"
    "beam INT NOT NULL DEFAULT 0,"
    "screen INT NOT NULL DEFAULT 0,"
    "tube INT NOT NULL DEFAULT 0,"
    "missiles INT NOT NULL DEFAULT 0,"
    "sr INT NOT NULL DEFAULT 0,"
    "pd_spent INT NOT NULL DEFAULT 0,"
    "at_system VARCHAR(16) DEFAULT NULL,"
    "at_hex VARCHAR(8) DEFAULT NULL,"
    "racked_in VARCHAR(4) DEFAULT NULL,"
    "destroyed_at TIMESTAMP NULL DEFAULT NULL,"
    "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,"
    "UNIQUE KEY uniq_ship (game_id, owner, ship_code),"
    "FOREIGN KEY (game_id) REFERENCES games(id)"
    ")",
    
    // sightings
    "CREATE TABLE IF NOT EXISTS sightings ("
    "id BIGINT AUTO_INCREMENT PRIMARY KEY,"
    "game_id INT NOT NULL,"
    "observer_owner CHAR(1) NOT NULL,"
    "subject_owner CHAR(1) NOT NULL,"
    "ship_code VARCHAR(4) NOT NULL,"
    "ship_name VARCHAR(32) NOT NULL,"
    "ship_type CHAR(1) NOT NULL,"
    "at_system VARCHAR(16) NOT NULL,"
    "last_seen_turn VARCHAR(8) NOT NULL,"
    "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,"
    "UNIQUE KEY uniq_sighting (game_id, observer_owner, ship_code),"
    "FOREIGN KEY (game_id) REFERENCES games(id)"
    ")",
    
    // star_systems
    "CREATE TABLE IF NOT EXISTS star_systems ("
    "map_id INT NOT NULL DEFAULT 1,"
    "hex_id VARCHAR(8) NOT NULL,"
    "name VARCHAR(64) NOT NULL,"
    "is_base TINYINT NOT NULL DEFAULT 0,"
    "base_owner CHAR(1) NULL,"
    "PRIMARY KEY (map_id, name),"
    "INDEX (map_id, hex_id),"
    "INDEX (map_id, is_base),"
    "INDEX (map_id, base_owner)"
    ")",
    
    // warplines
    "CREATE TABLE IF NOT EXISTS warplines ("
    "map_id INT NOT NULL DEFAULT 1,"
    "id INT NOT NULL AUTO_INCREMENT,"
    "a_hex VARCHAR(8) NOT NULL,"
    "b_hex VARCHAR(8) NOT NULL,"
    "PRIMARY KEY (id),"
    "INDEX (map_id),"
    "INDEX (map_id, a_hex),"
    "INDEX (map_id, b_hex)"
    ")",
    
    // hexes
    "CREATE TABLE IF NOT EXISTS hexes ("
    "map_id INT NOT NULL DEFAULT 1,"
    "hex_id VARCHAR(8) NOT NULL,"
    "q INT NOT NULL,"
    "r INT NOT NULL,"
    "PRIMARY KEY (map_id, hex_id),"
    "INDEX (map_id, q),"
    "INDEX (map_id, r)"
    ")",
    
    // warpline_hexes
    "CREATE TABLE IF NOT EXISTS warpline_hexes ("
    "map_id INT NOT NULL DEFAULT 1,"
    "warpline_id INT NOT NULL,"
    "hex_id VARCHAR(8) NOT NULL,"
    "PRIMARY KEY (map_id, warpline_id, hex_id),"
    "INDEX (map_id, hex_id),"
    "INDEX (map_id, warpline_id)"
    ")",
    
    // combat_state
    "CREATE TABLE IF NOT EXISTS combat_state ("
    "game_id INT NOT NULL,"
    "hex_id VARCHAR(8) NOT NULL,"
    "round INT NOT NULL DEFAULT 1,"
    "stage INT NOT NULL DEFAULT 0,"
    "attacker_remains BOOLEAN NOT NULL DEFAULT 0,"
    "stalemate_counter INT NOT NULL DEFAULT 0,"
    "pending_damage_json TEXT DEFAULT NULL,"
    "damage_assigned_A BOOLEAN NOT NULL DEFAULT 0,"
    "damage_assigned_B BOOLEAN NOT NULL DEFAULT 0,"
    "last_log TEXT DEFAULT NULL,"
    "PRIMARY KEY (game_id, hex_id),"
    "FOREIGN KEY (game_id) REFERENCES games(id)"
    ")",
    
    // combat_orders
    "CREATE TABLE IF NOT EXISTS combat_orders ("
    "game_id INT NOT NULL,"
    "ship_code VARCHAR(4) NOT NULL,"
    "owner CHAR(1) NOT NULL,"
    "round INT NOT NULL,"
    "tactic CHAR(1) NOT NULL DEFAULT 'A',"
    "target_id VARCHAR(4) DEFAULT NULL,"
    "power_d INT NOT NULL DEFAULT 0,"
    "power_b INT NOT NULL DEFAULT 0,"
    "power_s INT NOT NULL DEFAULT 0,"
    "power_t INT NOT NULL DEFAULT 0,"
    "missiles_data TEXT DEFAULT NULL,"
    "committed BOOLEAN NOT NULL DEFAULT 0,"
    "PRIMARY KEY (game_id, owner, ship_code, round),"
    "FOREIGN KEY (game_id) REFERENCES games(id) ON DELETE CASCADE,"
    "FOREIGN KEY (game_id, owner, ship_code) REFERENCES ships(game_id, owner, ship_code) ON DELETE CASCADE"
    ")",
    
    // telemetry_queue
    "CREATE TABLE IF NOT EXISTS telemetry_queue ("
    "id BIGINT AUTO_INCREMENT PRIMARY KEY,"
    "game_id INT NOT NULL,"
    "target_player ENUM('A','B','BOTH') NOT NULL,"
    "message TEXT NOT NULL,"
    "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,"
    "sent_at TIMESTAMP NULL DEFAULT NULL,"
    "FOREIGN KEY (game_id) REFERENCES games(id),"
    "INDEX (game_id, target_player, sent_at)"
    ")"
};

// Seed data INSERT statements (for --seed)
static const std::vector<std::string> SEED_STATEMENTS = {
    // Star Systems (map_id, hex_id, name, is_base, base_owner)
    "INSERT IGNORE INTO star_systems(map_id,hex_id,name,is_base,base_owner) VALUES(1,'0307','SONAL',1,NULL)",
    "INSERT IGNORE INTO star_systems(map_id,hex_id,name,is_base,base_owner) VALUES(1,'0606','UR',1,NULL)",
    "INSERT IGNORE INTO star_systems(map_id,hex_id,name,is_base,base_owner) VALUES(1,'0804','LARSU',1,NULL)",
    "INSERT IGNORE INTO star_systems(map_id,hex_id,name,is_base,base_owner) VALUES(1,'0611','SIPPUR',0,NULL)",
    "INSERT IGNORE INTO star_systems(map_id,hex_id,name,is_base,base_owner) VALUES(1,'0710','ERECH',0,NULL)",
    "INSERT IGNORE INTO star_systems(map_id,hex_id,name,is_base,base_owner) VALUES(1,'0908','CALAH',0,NULL)",
    "INSERT IGNORE INTO star_systems(map_id,hex_id,name,is_base,base_owner) VALUES(1,'0813','BYBLOS',0,NULL)",
    "INSERT IGNORE INTO star_systems(map_id,hex_id,name,is_base,base_owner) VALUES(1,'1011','ADAB',0,NULL)",
    "INSERT IGNORE INTO star_systems(map_id,hex_id,name,is_base,base_owner) VALUES(1,'1207','SUSA',0,NULL)",
    "INSERT IGNORE INTO star_systems(map_id,hex_id,name,is_base,base_owner) VALUES(1,'1014','UBAID',0,NULL)",
    "INSERT IGNORE INTO star_systems(map_id,hex_id,name,is_base,base_owner) VALUES(1,'1310','NIPPUR',0,NULL)",
    "INSERT IGNORE INTO star_systems(map_id,hex_id,name,is_base,base_owner) VALUES(1,'1313','KHAFA',0,NULL)",
    "INSERT IGNORE INTO star_systems(map_id,hex_id,name,is_base,base_owner) VALUES(1,'1415','MARI',0,NULL)",
    "INSERT IGNORE INTO star_systems(map_id,hex_id,name,is_base,base_owner) VALUES(1,'1614','LAGASH',0,NULL)",
    "INSERT IGNORE INTO star_systems(map_id,hex_id,name,is_base,base_owner) VALUES(1,'1712','ASSUR',0,NULL)",
    "INSERT IGNORE INTO star_systems(map_id,hex_id,name,is_base,base_owner) VALUES(1,'1419','SUMARRA',0,NULL)",
    "INSERT IGNORE INTO star_systems(map_id,hex_id,name,is_base,base_owner) VALUES(1,'1616','ELAM',0,NULL)",
    "INSERT IGNORE INTO star_systems(map_id,hex_id,name,is_base,base_owner) VALUES(1,'1814','JARMO',0,NULL)",
    "INSERT IGNORE INTO star_systems(map_id,hex_id,name,is_base,base_owner) VALUES(1,'1719','UMMA',0,NULL)",
    "INSERT IGNORE INTO star_systems(map_id,hex_id,name,is_base,base_owner) VALUES(1,'1817','GIRSU',0,NULL)",
    "INSERT IGNORE INTO star_systems(map_id,hex_id,name,is_base,base_owner) VALUES(1,'1622','ISIN',0,NULL)",
    "INSERT IGNORE INTO star_systems(map_id,hex_id,name,is_base,base_owner) VALUES(1,'1922','SUMER',0,NULL)",
    "INSERT IGNORE INTO star_systems(map_id,hex_id,name,is_base,base_owner) VALUES(1,'2020','AKKAD',0,NULL)",
    "INSERT IGNORE INTO star_systems(map_id,hex_id,name,is_base,base_owner) VALUES(1,'2118','KISH',0,NULL)",
    "INSERT IGNORE INTO star_systems(map_id,hex_id,name,is_base,base_owner) VALUES(1,'2318','ERIDU',0,NULL)",
    "INSERT IGNORE INTO star_systems(map_id,hex_id,name,is_base,base_owner) VALUES(1,'2125','NINEVEH',1,NULL)",
    "INSERT IGNORE INTO star_systems(map_id,hex_id,name,is_base,base_owner) VALUES(1,'2223','BABYLON',1,NULL)",
    "INSERT IGNORE INTO star_systems(map_id,hex_id,name,is_base,base_owner) VALUES(1,'2622','UGARIT',1,NULL)",
    
    // Warplines (map_id, a_hex, b_hex)
    "INSERT INTO warplines(map_id,a_hex,b_hex) VALUES(1,'0307','0611')",
    "INSERT INTO warplines(map_id,a_hex,b_hex) VALUES(1,'0710','0606')",
    "INSERT INTO warplines(map_id,a_hex,b_hex) VALUES(1,'0710','1011')",
    "INSERT INTO warplines(map_id,a_hex,b_hex) VALUES(1,'1011','0813')",
    "INSERT INTO warplines(map_id,a_hex,b_hex) VALUES(1,'1011','1313')",
    "INSERT INTO warplines(map_id,a_hex,b_hex) VALUES(1,'0804','1207')",
    "INSERT INTO warplines(map_id,a_hex,b_hex) VALUES(1,'0908','1310')",
    "INSERT INTO warplines(map_id,a_hex,b_hex) VALUES(1,'1207','1310')",
    "INSERT INTO warplines(map_id,a_hex,b_hex) VALUES(1,'1310','1712')",
    "INSERT INTO warplines(map_id,a_hex,b_hex) VALUES(1,'1310','1614')",
    "INSERT INTO warplines(map_id,a_hex,b_hex) VALUES(1,'1614','1616')",
    "INSERT INTO warplines(map_id,a_hex,b_hex) VALUES(1,'1614','1712')",
    "INSERT INTO warplines(map_id,a_hex,b_hex) VALUES(1,'1814','2118')",
    "INSERT INTO warplines(map_id,a_hex,b_hex) VALUES(1,'2118','2318')",
    "INSERT INTO warplines(map_id,a_hex,b_hex) VALUES(1,'2318','2622')",
    "INSERT INTO warplines(map_id,a_hex,b_hex) VALUES(1,'2118','2020')",
    "INSERT INTO warplines(map_id,a_hex,b_hex) VALUES(1,'1014','1415')",
    "INSERT INTO warplines(map_id,a_hex,b_hex) VALUES(1,'1415','1719')",
    "INSERT INTO warplines(map_id,a_hex,b_hex) VALUES(1,'1014','1419')",
    "INSERT INTO warplines(map_id,a_hex,b_hex) VALUES(1,'1419','1719')",
    "INSERT INTO warplines(map_id,a_hex,b_hex) VALUES(1,'1719','1817')",
    "INSERT INTO warplines(map_id,a_hex,b_hex) VALUES(1,'1719','1922')",
    "INSERT INTO warplines(map_id,a_hex,b_hex) VALUES(1,'2223','1922')",
    "INSERT INTO warplines(map_id,a_hex,b_hex) VALUES(1,'1622','2125')"
};

// Hexes seed data - generated from hexes.csv
// Using multi-row INSERT for efficiency
static const std::vector<std::string> HEXES_SEED = {
    "INSERT IGNORE INTO hexes(map_id,hex_id,q,r) VALUES"
    "(1,'0107',1,7),(1,'0206',2,6),(1,'0305',3,5),(1,'0404',4,4),(1,'0503',5,3),(1,'0602',6,2),(1,'0701',7,1),"
    "(1,'0108',1,8),(1,'0207',2,7),(1,'0306',3,6),(1,'0405',4,5),(1,'0504',5,4),(1,'0603',6,3),(1,'0702',7,2),(1,'0801',8,1),"
    "(1,'0208',2,8),(1,'0307',3,7),(1,'0406',4,6),(1,'0505',5,5),(1,'0604',6,4),(1,'0703',7,3),(1,'0802',8,2),"
    "(1,'0209',2,9),(1,'0308',3,8),(1,'0407',4,7),(1,'0506',5,6),(1,'0605',6,5),(1,'0704',7,4),(1,'0803',8,3),(1,'0902',9,2),"
    "(1,'0309',3,9),(1,'0408',4,8),(1,'0507',5,7),(1,'0606',6,6),(1,'0705',7,5),(1,'0804',8,4),(1,'0903',9,3),"
    "(1,'0310',3,10),(1,'0409',4,9),(1,'0508',5,8),(1,'0607',6,7),(1,'0706',7,6),(1,'0805',8,5),(1,'0904',9,4),(1,'1003',10,3),"
    "(1,'0410',4,10),(1,'0509',5,9),(1,'0608',6,8),(1,'0707',7,7),(1,'0806',8,6),(1,'0905',9,5),(1,'1004',10,4),"
    "(1,'0411',4,11),(1,'0510',5,10),(1,'0609',6,9),(1,'0708',7,8),(1,'0807',8,7),(1,'0906',9,6),(1,'1005',10,5),(1,'1104',11,4),"
    "(1,'0511',5,11),(1,'0610',6,10),(1,'0709',7,9),(1,'0808',8,8),(1,'0907',9,7),(1,'1006',10,6),(1,'1105',11,5),"
    "(1,'0512',5,12),(1,'0611',6,11),(1,'0710',7,10),(1,'0809',8,9),(1,'0908',9,8),(1,'1007',10,7),(1,'1106',11,6),(1,'1205',12,5),"
    "(1,'0612',6,12),(1,'0711',7,11),(1,'0810',8,10),(1,'0909',9,9),(1,'1008',10,8),(1,'1107',11,7),(1,'1206',12,6),"
    "(1,'0613',6,13),(1,'0712',7,12),(1,'0811',8,11),(1,'0910',9,10),(1,'1009',10,9),(1,'1108',11,8),(1,'1207',12,7),(1,'1306',13,6),"
    "(1,'0713',7,13),(1,'0812',8,12),(1,'0911',9,11),(1,'1010',10,10),(1,'1109',11,9),(1,'1208',12,8),(1,'1307',13,7),"
    "(1,'0714',7,14),(1,'0813',8,13),(1,'0912',9,12),(1,'1011',10,11),(1,'1110',11,10),(1,'1209',12,9),(1,'1308',13,8),(1,'1407',14,7),"
    "(1,'0814',8,14),(1,'0913',9,13),(1,'1012',10,12),(1,'1111',11,11),(1,'1210',12,10),(1,'1309',13,9),(1,'1408',14,8),"
    "(1,'0815',8,15),(1,'0914',9,14),(1,'1013',10,13),(1,'1112',11,12),(1,'1211',12,11),(1,'1310',13,10),(1,'1409',14,9),(1,'1508',15,8),"
    "(1,'0915',9,15),(1,'1014',10,14),(1,'1113',11,13),(1,'1212',12,12),(1,'1311',13,11),(1,'1410',14,10),(1,'1509',15,9),"
    "(1,'0916',9,16),(1,'1015',10,15),(1,'1114',11,14),(1,'1213',12,13),(1,'1312',13,12),(1,'1411',14,11),(1,'1510',15,10),(1,'1609',16,9),"
    "(1,'1016',10,16),(1,'1115',11,15),(1,'1214',12,14),(1,'1313',13,13),(1,'1412',14,12),(1,'1511',15,11),(1,'1610',16,10),"
    "(1,'1017',10,17),(1,'1116',11,16),(1,'1215',12,15),(1,'1314',13,14),(1,'1413',14,13),(1,'1512',15,12),(1,'1611',16,11),(1,'1710',17,10),"
    "(1,'1117',11,17),(1,'1216',12,16),(1,'1315',13,15),(1,'1414',14,14),(1,'1513',15,13),(1,'1612',16,12),(1,'1711',17,11),"
    "(1,'1118',11,18),(1,'1217',12,17),(1,'1316',13,16),(1,'1415',14,15),(1,'1514',15,14),(1,'1613',16,13),(1,'1712',17,12),(1,'1811',18,11),"
    "(1,'1218',12,18),(1,'1317',13,17),(1,'1416',14,16),(1,'1515',15,15),(1,'1614',16,14),(1,'1713',17,13),(1,'1812',18,12),"
    "(1,'1219',12,19),(1,'1318',13,18),(1,'1417',14,17),(1,'1516',15,16),(1,'1615',16,15),(1,'1714',17,14),(1,'1813',18,13),(1,'1912',19,12),"
    "(1,'1319',13,19),(1,'1418',14,18),(1,'1517',15,17),(1,'1616',16,16),(1,'1715',17,15),(1,'1814',18,14),(1,'1913',19,13),"
    "(1,'1320',13,20),(1,'1419',14,19),(1,'1518',15,18),(1,'1617',16,17),(1,'1716',17,16),(1,'1815',18,15),(1,'1914',19,14),(1,'2013',20,13),"
    "(1,'1420',14,20),(1,'1519',15,19),(1,'1618',16,18),(1,'1717',17,17),(1,'1816',18,16),(1,'1915',19,15),(1,'2014',20,14),"
    "(1,'1421',14,21),(1,'1520',15,20),(1,'1619',16,19),(1,'1718',17,18),(1,'1817',18,17),(1,'1916',19,16),(1,'2015',20,15),(1,'2114',21,14),"
    "(1,'1521',15,21),(1,'1620',16,20),(1,'1719',17,19),(1,'1818',18,18),(1,'1917',19,17),(1,'2016',20,16),(1,'2115',21,15),"
    "(1,'1522',15,22),(1,'1621',16,21),(1,'1720',17,20),(1,'1819',18,19),(1,'1918',19,18),(1,'2017',20,17),(1,'2116',21,16),(1,'2215',22,15),"
    "(1,'1622',16,22),(1,'1721',17,21),(1,'1820',18,20),(1,'1919',19,19),(1,'2018',20,18),(1,'2117',21,17),(1,'2216',22,16),"
    "(1,'1623',16,23),(1,'1722',17,22),(1,'1821',18,21),(1,'1920',19,20),(1,'2019',20,19),(1,'2118',21,18),(1,'2217',22,17),(1,'2316',23,16),"
    "(1,'1723',17,23),(1,'1822',18,22),(1,'1921',19,21),(1,'2020',20,20),(1,'2119',21,19),(1,'2218',22,18),(1,'2317',23,17),"
    "(1,'1724',17,24),(1,'1823',18,23),(1,'1922',19,22),(1,'2021',20,21),(1,'2120',21,20),(1,'2219',22,19),(1,'2318',23,18),(1,'2417',24,17),"
    "(1,'1824',18,24),(1,'1923',19,23),(1,'2022',20,22),(1,'2121',21,21),(1,'2220',22,20),(1,'2319',23,19),(1,'2418',24,18),"
    "(1,'1825',18,25),(1,'1924',19,24),(1,'2023',20,23),(1,'2122',21,22),(1,'2221',22,21),(1,'2320',23,20),(1,'2419',24,19),(1,'2518',25,18),"
    "(1,'1925',19,25),(1,'2024',20,24),(1,'2123',21,23),(1,'2222',22,22),(1,'2321',23,21),(1,'2420',24,20),(1,'2519',25,19),"
    "(1,'1926',19,26),(1,'2025',20,25),(1,'2124',21,24),(1,'2223',22,23),(1,'2322',23,22),(1,'2421',24,21),(1,'2520',25,20),(1,'2619',26,19),"
    "(1,'2026',20,26),(1,'2125',21,25),(1,'2224',22,24),(1,'2323',23,23),(1,'2422',24,22),(1,'2521',25,21),(1,'2620',26,20),"
    "(1,'2027',20,27),(1,'2126',21,26),(1,'2225',22,25),(1,'2324',23,24),(1,'2423',24,23),(1,'2522',25,22),(1,'2621',26,21),(1,'2720',27,20),"
    "(1,'2127',21,27),(1,'2226',22,26),(1,'2325',23,25),(1,'2424',24,24),(1,'2523',25,23),(1,'2622',26,22),(1,'2721',27,21),"
    "(1,'2128',21,28),(1,'2227',22,27),(1,'2326',23,26),(1,'2425',24,25),(1,'2524',25,24),(1,'2623',26,23),(1,'2722',27,22),(1,'2821',28,21),"
    "(1,'2228',22,28),(1,'2327',23,27),(1,'2426',24,26),(1,'2525',25,25),(1,'2624',26,24),(1,'2723',27,23),(1,'2822',28,22)"
};

// Warpline hexes seed data - generated from warpline_hexes.csv
static const std::vector<std::string> WARPLINE_HEXES_SEED = {
    "INSERT IGNORE INTO warpline_hexes(map_id,warpline_id,hex_id) VALUES"
    "(1,1,'0307'),(1,1,'0308'),(1,1,'0408'),(1,1,'0409'),(1,1,'0509'),(1,1,'0510'),(1,1,'0610'),(1,1,'0611'),"
    "(1,2,'0710'),(1,2,'0709'),(1,2,'0708'),(1,2,'0608'),(1,2,'0607'),(1,2,'0606'),"
    "(1,3,'0710'),(1,3,'0810'),(1,3,'0811'),(1,3,'0911'),(1,3,'1011'),"
    "(1,4,'1011'),(1,4,'0912'),(1,4,'0813'),"
    "(1,5,'1011'),(1,5,'1111'),(1,5,'1112'),(1,5,'1212'),(1,5,'1213'),(1,5,'1313'),"
    "(1,6,'0804'),(1,6,'0904'),(1,6,'0905'),(1,6,'1005'),(1,6,'1006'),(1,6,'1106'),(1,6,'1107'),(1,6,'1207'),"
    "(1,7,'0908'),(1,7,'1008'),(1,7,'1009'),(1,7,'1109'),(1,7,'1209'),(1,7,'1210'),(1,7,'1310'),"
    "(1,8,'1207'),(1,8,'1208'),(1,8,'1209'),(1,8,'1309'),(1,8,'1310'),"
    "(1,9,'1310'),(1,9,'1410'),(1,9,'1411'),(1,9,'1511'),(1,9,'1611'),(1,9,'1612'),(1,9,'1712'),"
    "(1,10,'1310'),(1,10,'1311'),(1,10,'1411'),(1,10,'1412'),(1,10,'1512'),(1,10,'1513'),(1,10,'1613'),(1,10,'1614'),"
    "(1,11,'1614'),(1,11,'1615'),(1,11,'1616'),"
    "(1,12,'1614'),(1,12,'1613'),(1,12,'1712'),"
    "(1,13,'1814'),(1,13,'1815'),(1,13,'1915'),(1,13,'1916'),(1,13,'2016'),(1,13,'2017'),(1,13,'2117'),(1,13,'2118'),"
    "(1,14,'2118'),(1,14,'2218'),(1,14,'2318'),"
    "(1,15,'2318'),(1,15,'2319'),(1,15,'2419'),(1,15,'2420'),(1,15,'2520'),(1,15,'2521'),(1,15,'2621'),(1,15,'2622'),"
    "(1,16,'2118'),(1,16,'2019'),(1,16,'2020'),"
    "(1,17,'1014'),(1,17,'1114'),(1,17,'1214'),(1,17,'1215'),(1,17,'1315'),(1,17,'1415'),"
    "(1,18,'1415'),(1,18,'1416'),(1,18,'1516'),(1,18,'1517'),(1,18,'1617'),(1,18,'1618'),(1,18,'1718'),(1,18,'1719'),"
    "(1,19,'1014'),(1,19,'1015'),(1,19,'1115'),(1,19,'1116'),(1,19,'1216'),(1,19,'1217'),(1,19,'1317'),(1,19,'1318'),(1,19,'1418'),(1,19,'1419'),"
    "(1,20,'1419'),(1,20,'1519'),(1,20,'1619'),(1,20,'1719'),"
    "(1,21,'1719'),(1,21,'1818'),(1,21,'1817'),"
    "(1,22,'1719'),(1,22,'1720'),(1,22,'1820'),(1,22,'1821'),(1,22,'1921'),(1,22,'1922'),"
    "(1,23,'2223'),(1,23,'2123'),(1,23,'2023'),(1,23,'2022'),(1,23,'1922'),"
    "(1,24,'1622'),(1,24,'1722'),(1,24,'1723'),(1,24,'1823'),(1,24,'1824'),(1,24,'1924'),(1,24,'2024'),(1,24,'2025'),(1,24,'2125')"
};

#endif // __LOAD_H__
