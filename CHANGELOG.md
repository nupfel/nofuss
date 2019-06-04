# NoFUSS change log

The format is based on [Keep a Changelog](http://keepachangelog.com/)
and this project adheres to [Semantic Versioning](http://semver.org/).

## [0.3.0] 2019-06-04
### Added
- Added more version checks (lt, le, eq, ge, gt)

### Changed
- Changed build to build_not in json file
- Autocreate spiffs tag in response if there is none

### Deprecated
- Use `ge` and `lt` instead of `min` and `max` in filters
  
## [0.2.7] 2019-05-26
### Fixed
- Make it backwards compatible using HTTPUPDATE_1_2_COMPATIBLE flag
- Added build header to log

## [0.2.6] 2019-05-25
### Added
- Option to define multiple MAC addresses for the same origin
- Option to define a "build" filter for dev versions
- Added docker-compose image

### Changed
- Documentation updates
- Updated HTTPClient and HTTPUpdateClient libraries

## [0.2.5] 2017-08-28
### Changed
- Report HTTP error for callbacks

## [0.2.4] 2017-08-28
### Changed
- Some changes in examples
- Small changes to code (increased timeout, do not reuse connection)

## [0.2.3] 2017-05-06
### Fixed
- Force payload response to be an HTTP code 200

## [0.2.2] 2017-01-06
### Added
- Default values for MAC, MIN and MAX filters

### Fixed
- Added trailing slash to example URL

## [0.2.1] 2017-01-05
### Fixed
- Including Stream.h to fix build error with latest ArduinoJson release (issue #1)

## [0.2.0] 2016-08-10

### Added
- Using headers to pass information
- MAC filtering support

## [0.1.2] 2016-09-18

### Changed
- Check if callback has been defined before calling it

## [0.1.1] 2016-08-10

### Changed
- Do not automatically reboot on update

## [0.1.0] 2016-07-30
- Initial working version
