pm25 = thingSpeakRead(170228, 'Fields', 2, 'NumMinutes', 21, 'ReadKey', 'C1XHT1J1R181MOCE');

% Calculate the average humidity
avgPm25 = mean(pm25);
if(avgPm25 > 5)
    avgPm25 = avgPm25 - 20 + (130 / avgPm25) ;
end

% Write the average humidity to another channel specified by the
% 'writeChannelID' variable

thingSpeakWrite(229635, avgPm25, 'writekey', '4ABB1FXW2EY12I40');

%// AQI formula: https://en.wikipedia.org/wiki/Air_Quality_Index#United_States
function output = toAQI(I_high, I_low, C_high, C_low, C)
  aqiResult = (I_high - I_low) * (C - C_low) / (C_high - C_low) + I_low
end

%//thanks to https://gist.github.com/nfjinjing/8d63012c18feea3ed04e
function output = calculateAQI25(density) 
  d10 = density * 10;
  
  if d10 <= 0
    output = 0;
  elseif d10 <= 120
    output =  toAQI(50, 0, 120, 0, d10);
  elseif d10 <= 354
    output =  toAQI(100, 51, 354, 121, d10);
  elseif d10 <= 554
    output =  toAQI(150, 101, 554, 355, d10);
  elseif d10 <= 1504
    output =  toAQI(200, 151, 1504, 555, d10);
  elseif d10 <= 2504
    output =  toAQI(300, 201, 2504, 1505, d10);
  elseif d10 <= 3504
    output =  toAQI(400, 301, 3504, 2505, d10);
  elseif d10 <= 5004
    output =  toAQI(500, 401, 5004, 3505, d10);
  elseif d10 <= 10000
    output = toAQI(1000, 501, 10000, 5005, d10);
  else
    output = 1001;
  end
end
