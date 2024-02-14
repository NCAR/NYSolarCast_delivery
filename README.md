# NYSolarCast_delivery
This repo will be used to facilitate delivery of the NY Solar Cast system.

THIS REPO IS NOT DESIGNED TO AUTOMATICALLY RUN UPON DOWNLOAD AND INSTALLATION OF THE CRONTAB.  USER WILL NEED TO CONFIGURE FARM INFORMATION, AS WELL AS TIMING OF THE CRONTAB AND WITHIN THE PYTHON SCRIPTS THAT LOOK FOR SPECIFIC INPUTS BASED ON TIME OF DAY.  TIMING PARAMETERS WILL DEPEND ON THE LATENCY OF THE INPUT MODEL FILES (WRF/HRRR).

Please see the DOCS directory for a system diagram and a document giving a high-level outline to the scripts in the NYSolarCast repository. Additionally, these technical reports were prepared that describe in greater depth the various components of NYSolarCast:

- WRF-Solar System Description: https://www.epri.com/research/programs/067417/results/3002025151

- Blended Solar Irradiance Forecast Component Description: https://www.epri.com/research/programs/067417/results/3002025152

- Solar Power Forecast Component Description: https://www.epri.com/research/programs/067417/results/3002025153

- Overall Final Project Report: https://www.epri.com/research/programs/067417/results/3002026232

A peer-reviewed journal article describing NYSolarCast and its forecast performance has been conditionally accepted and is in revision. A reference will be provided here if and when the article is accepted and published.
