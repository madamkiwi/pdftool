# simple pdf manipulation tool

split files with a "config" file, a comma-separated configuration file which follows this format: name,pattern,page_num
```
	process	<input file>
```
split/merge without config
```
	split	<input file>	<start page #>	<end page #>	<output file>
	merge	<input dir>	<output file>
```
