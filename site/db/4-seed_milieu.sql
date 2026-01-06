-- OK
USE khdb;
START TRANSACTION;

LOAD DATA LOCAL INFILE 'milieu/anomalies.csv'
INTO TABLE system_anomalies
FIELDS TERMINATED BY ','
LINES TERMINATED BY '\n'
(id,system_name,anomaly_type,name,effect,discovery_text);

LOAD DATA LOCAL INFILE 'milieu/asteroid_belts.csv'
INTO TABLE system_asteroid_belts
FIELDS TERMINATED BY ','
LINES TERMINATED BY '\n'
(id,system_name,designation,orbital_position,density,composition,hazard_level,notes);


LOAD DATA LOCAL INFILE 'milieu/facilities.csv'
INTO TABLE system_facilities
FIELDS TERMINATED BY ','
LINES TERMINATED BY '\n'
(id,location_type,location_id,facility_type,name,capacity,owner,operational,notes);


LOAD DATA LOCAL INFILE 'milieu/grimoire_rumors.csv'
INTO TABLE system_grimoire_rumors
FIELDS TERMINATED BY ','
LINES TERMINATED BY '\n'
(system_name,rumor_text,knowledge_level);


LOAD DATA LOCAL INFILE 'milieu/moons.csv'
INTO TABLE system_moons
FIELDS TERMINATED BY ','
LINES TERMINATED BY '\n'
(id,planet_id,designation,common_name,moon_type,size,notable_feature,notes);


LOAD DATA LOCAL INFILE 'milieu/planets.csv'
INTO TABLE system_planets
FIELDS TERMINATED BY ','
LINES TERMINATED BY '\n'
(id,system_name,star_id,orbital_position,designation,common_name,planet_type,atmosphere,hydrosphere,biosphere,habitability,notes);


LOAD DATA LOCAL INFILE 'milieu/populations.csv'
INTO TABLE system_populations
FIELDS TERMINATED BY ','
LINES TERMINATED BY '\n'
(id,location_type,location_id,species_id,pop_class,population_millions,tech_level,government,disposition,notes);


LOAD DATA LOCAL INFILE 'milieu/resources.csv'
INTO TABLE system_resources
FIELDS TERMINATED BY ','
LINES TERMINATED BY '\n'
(id,location_type,location_id,resource_type,abundance,extraction_difficulty,notes);


LOAD DATA LOCAL INFILE 'milieu/species.csv'
INTO TABLE system_species
FIELDS TERMINATED BY ','
LINES TERMINATED BY '\n'
(id,name,classification,homeworld_system,physiology,psychology,special_ability);


LOAD DATA LOCAL INFILE 'milieu/stars.csv'
INTO TABLE system_stars
FIELDS TERMINATED BY ','
LINES TERMINATED BY '\n'
(id,system_name,designation,star_class,luminosity,color,age_gy,notes);

COMMIT;

