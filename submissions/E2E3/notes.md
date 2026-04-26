E2

I had trouble connecting node red to influxdb, I ended up having to change the auth key to a custom key and had to update the ip address in node red. Once all that was fixed the data appeared in influxdb no problem.

Connecting Grafana to Influx was much easier since i had already dealt with the issues when doing nodered.

In the future things id like to do would be to add more nodes to the similation and have them connected to each other as they would in the real world. Then in the dashboard show a heatmap of the actual area they cover instead of a single time series heatmap.

I had no problems setting up and running the jupyter notebook for the data analysis. Realistically since the data is entirely simulated the analysis was quite easy and the coralations were high as to be expected from fake data.