%// Euro AQI Standard
%// AQI formula: https://en.wikipedia.org/wiki/Air_Quality_Index#United_States
function output = toAQI(I_high, I_low, C_high, C_low, C)
  aqiResult = (I_high - I_low) * (C - C_low) / (C_high - C_low) + I_low
end

%//thanks to https://gist.github.com/nfjinjing/8d63012c18feea3ed04e
function output = calculateAQI25(density) 
  d10 = density * 10;
    
  if d10 <= 150
    output =  toAQI(25, 0, 150, 0, d10);
  elseif d10 <= 300
    output =  toAQI(50, 26, 300, 151, d10);
  elseif d10 <= 500
    output =  toAQI(75, 51, 500, 301, d10);
  elseif d10 <= 1000
    output =  toAQI(100, 76, 1000, 501, d10);
  elseif d10 <= 3000
    output =  toAQI(300, 101, 3000, 1001, d10);
  else
    output = 1001;
  end
end

%// Polution levels
function output = aqi25captions(density)     
  if density <= 25
    output =  'Perfect =)';
  elseif density <= 50
    output =  'Good :)';
  elseif density <= 75
    output =  'Moderate :|';
  elseif density <= 100
    output =  'Unhealthy :(';
  elseif density <= 200
    output =  'Bad :O';
  else
    output =  'Hazardous X(';
  end
end
