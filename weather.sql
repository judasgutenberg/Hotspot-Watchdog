

CREATE TABLE weather_data(
weather_data_id INT AUTO_INCREMENT PRIMARY KEY,
location_id INT NULL,
recorded DATETIME,
temperature DECIMAL(6,3) NULL,
pressure DECIMAL(9,4) NULL,
humidity DECIMAL(6,3) NULL,
wind_direction INT NULL,
precipitation INT NULL,
wind_speed DECIMAL(8,3) NULL,
wind_increment INT NULL,
gas_metric DECIMAL(15,4) NULL
)
