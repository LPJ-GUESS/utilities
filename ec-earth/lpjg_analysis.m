function lpjg_analysis(outputdir, f_year, t_year)

%outputdir='test'; 
%f_year='2000';
%t_year='2010';

tslice_years = strcat(f_year,'to',t_year);
nr = 1;
save_fig = 1;

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%%% TIMESEIRES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%


%%%%%%%%%%%%%%%%%
%%%% C pools
%%%%%%%%%%%%%%%%%
file = 'C~pools'
Units = '[kg~C~m^{-2}]';
filename = 'cpool_Pg';

globaltimeseries(nr,outputdir,filename,file,Units,save_fig,1,'VegC')
globaltimeseries(nr,outputdir,filename,file,Units,save_fig,1,'SoilC')
globaltimeseries(nr,outputdir,filename,file,Units,save_fig,1,'Total')

%%%%%%%%%%%%%%%%%
%%%% C fluxes
%%%%%%%%%%%%%%%%%
file = 'C~fluxes'
Units = '[kg~C~m^{-2}~year^{-1}]';
filename = 'cflux_Pg';

globaltimeseries(nr,outputdir,filename,file,Units,save_fig,1,'Veg')
globaltimeseries(nr,outputdir,filename,file,Units,save_fig,1,'Soil')
globaltimeseries(nr,outputdir,filename,file,Units,save_fig,5,'NEE')

%%%%%%%%%%%%%%%%%
%%%% FPC
%%%%%%%%%%%%%%%%%
file = 'Land~cover'
Units = '[-]';
filename = 'fpc_all';

globaltimeseries(nr,outputdir,filename,file,Units,save_fig,1,'FRACL')
globaltimeseries(nr,outputdir,filename,file,Units,save_fig,1,'FRACH')

%%%%%%%%%%%%%%%%%
%%%% N Sources
%%%%%%%%%%%%%%%%%
file = 'N~sources'
Units = '[kg~N~ha^{-1}~year^{-1}]';
filename = 'nsources_Tg';

globaltimeseries(nr,outputdir,filename,file,Units,save_fig,1,'dep')
globaltimeseries(nr,outputdir,filename,file,Units,save_fig,1,'fert')
globaltimeseries(nr,outputdir,filename,file,Units,save_fig,5,'netmin')
globaltimeseries(nr,outputdir,filename,file,Units,save_fig,5,'Total')

%%%%%%%%%%%%%%%%%
%%%% Seasonality
%%%%%%%%%%%%%%%%%
file = 'Seasonality'
filename = 'seasonality_all';

Units = '[^oC]';
globaltimeseries(nr,outputdir,filename,file,Units,save_fig,1,'temp_mean')

Units = '[mm year^{-1}]';
globaltimeseries(nr,outputdir,filename,file,Units,save_fig,1,'prec')





%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%%% GLOBAL DIFFERENCE MAPS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

%%%%%%%%%%%%%%%%%
%%%% C pools
%%%%%%%%%%%%%%%%%
file = 'C~pools'
filename = 'cpool';
Units = '[kg~C~m^{-2}]';

globalmap(nr,outputdir,filename,tslice_years,file,Units,save_fig,'VegC');
globalmap(nr,outputdir,filename,tslice_years,file,Units,save_fig,'SoilC');
globalmap(nr,outputdir,filename,tslice_years,file,Units,save_fig,'Total');

%%%%%%%%%%%%%%%%%
%%%% C flux
%%%%%%%%%%%%%%%%%

file = 'C~fluxes'
filename = 'cflux';
Units = '[kg~C~m^{-2}~year^{-1}]';

globalmap(nr,outputdir,filename,tslice_years,file,Units,save_fig,'Veg');
globalmap(nr,outputdir,filename,tslice_years,file,Units,save_fig,'Soil');
globalmap(nr,outputdir,filename,tslice_years,file,Units,save_fig,'NEE');

%%%%%%%%%%%%%%%%%
%%%% FPC
%%%%%%%%%%%%%%%%%

file = 'Land~cover'
filename = 'fpc';
Units = '[-]';

globalmap(nr,outputdir,filename,tslice_years,file,Units,save_fig,'FRACH');
globalmap(nr,outputdir,filename,tslice_years,file,Units,save_fig,'FRACL');

%%%%%%%%%%%%%%%%%
%%%% LAI
%%%%%%%%%%%%%%%%%

file = 'LAI'
filename = 'lai';
Units = '[m^{2}~m^{-2}]';

globalmap(nr,outputdir,filename,tslice_years,file,Units,save_fig,'BNE');
globalmap(nr,outputdir,filename,tslice_years,file,Units,save_fig,'BINE');
globalmap(nr,outputdir,filename,tslice_years,file,Units,save_fig,'BNS');
globalmap(nr,outputdir,filename,tslice_years,file,Units,save_fig,'TeNE');
globalmap(nr,outputdir,filename,tslice_years,file,Units,save_fig,'IBS');
globalmap(nr,outputdir,filename,tslice_years,file,Units,save_fig,'TeBS');
globalmap(nr,outputdir,filename,tslice_years,file,Units,save_fig,'TeBE');
globalmap(nr,outputdir,filename,tslice_years,file,Units,save_fig,'TrBE');
globalmap(nr,outputdir,filename,tslice_years,file,Units,save_fig,'TrIBE');
globalmap(nr,outputdir,filename,tslice_years,file,Units,save_fig,'TrBR');
globalmap(nr,outputdir,filename,tslice_years,file,Units,save_fig,'C3G');
globalmap(nr,outputdir,filename,tslice_years,file,Units,save_fig,'C4G');
globalmap(nr,outputdir,filename,tslice_years,file,Units,save_fig,'Natural_sum');
globalmap(nr,outputdir,filename,tslice_years,file,Units,save_fig,'Total');

%%%%%%%%%%%%%%%%%
%%%% N Sources
%%%%%%%%%%%%%%%%%

file = 'N~sources'
filename = 'nsources';
Units = '[kg~C~ha^{-1}~year^{-1}]';

globalmap(nr,outputdir,filename,tslice_years,file,Units,save_fig,'dep');
globalmap(nr,outputdir,filename,tslice_years,file,Units,save_fig,'fix');
globalmap(nr,outputdir,filename,tslice_years,file,Units,save_fig,'fert');
globalmap(nr,outputdir,filename,tslice_years,file,Units,save_fig,'netmin');
globalmap(nr,outputdir,filename,tslice_years,file,Units,save_fig,'Total');

%%%%%%%%%%%%%%%%%
%%%% Seasonality
%%%%%%%%%%%%%%%%%

file = 'Seasonality'
filename = 'seasonality';

Units = '[^oC]';
globalmap(nr,outputdir,filename,tslice_years,file,Units,save_fig,'temp_mean');

Units = '[mm~year^{-1}]';
globalmap(nr,outputdir,filename,tslice_years,file,Units,save_fig,'prec');

exit
end



%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%%
%%%      globaltimeseries()
%%%
function globaltimeseries(nr,outputdir,filename,file,Units,save_fig,runmean,var_name)

if (exist(strcat(outputdir,'/ref/',filename,'.txt'), 'file'))
    filenames=[string(strcat(outputdir,'/run/',filename,'.txt')),...
        string(strcat(outputdir,'/ref/',filename,'.txt'))];

    titles=[string('run'),string('ref')];
else
    filenames=[string(strcat(outputdir,'/run/',filename,'.txt'))];

    titles=[string('run')];
end

LT=1.2;
title_size = 18;
axis_size = 16;
legend_size = 16;
rez=300;
fig_with = 1000;
fig_height = 600;
x_label = 'Year';
leg_location = 'NorthEast';
plot_color=[' -k';' -r';' -b';' -g';' -m';' -y';' -c';' :k';' :r';' :b';' :g';' :m';' :y';' :c';'--k';'--r';'--b';'--g';'--m';'--y';'--c'];

nrofdatasets = length(filenames);

[var,dval] = load_LPJG_data(char(filenames(1)));
clear dval;

if (nargin > 7)
    if (~isempty(var_name))
        columns=find(strcmp(var_name,var));
    else
        error('ERROR: Sending in an empty variable name');
    end
else
    columns=2:length(var);
end

for col=columns
    
    legnr=0;
    
    fig=figure(nr);
    set(fig,'Visible','off');
    set(fig,'Color','White');
    set(fig,'Position', [100 100 fig_with fig_height]);
    hold on
    titlee=strcat('$',file,'~',strrep(var(col),'_','~'));
    if runmean > 1
        titlee=strcat(titlee,'~(',num2str(runmean),'~year~running~mean)$');
    else
        titlee=strcat(titlee,'$');
    end
    zz=title(strcat(titlee));
    set(zz,'FontSize',title_size);
    set(zz,'Interpreter','latex');
    xx=xlabel('$Year$','fontweight','bold');
    set(xx,'FontSize',axis_size);
    set(xx,'Interpreter','latex');
    yy=ylabel(strcat('$',var(col),'~',Units,'$'),'fontweight','bold');
    set(yy,'Interpreter','latex');
    set(yy,'FontSize',axis_size);
    x_range=[];
    y_range=[];
    
    for run=1:nrofdatasets

        [var_run,data] = load_LPJG_data(char(filenames(run)));

        col_run = find(strcmp(var(col),var_run));
    
        if (~isempty(col_run))
            
            data_sort = sortrows(data,1);
            
            if runmean > 1
                data_sort(:,col_run)=movmean(data_sort(:,col_run),runmean);
            end
            
            if (~isempty(x_range))
                x_range(1)=min(x_range(1),min(data_sort(:,1)));
                x_range(2)=max(x_range(2),max(data_sort(:,1)));
                y_range(1)=min(y_range(1),min(data_sort(:,col_run)));
                y_range(2)=max(y_range(2),max(data_sort(:,col_run)));
            else
                x_range(1)=min(data_sort(:,1));
                x_range(2)=max(data_sort(:,1));
                y_range(1)=min(data_sort(:,col_run));
                y_range(2)=max(data_sort(:,col_run));
            end
            plot_run=plot(data_sort(:,1),data_sort(:,col_run),plot_color(run,:));
            set(plot_run,'LineWidth',LT);
            
            legnr=legnr+1;
            legend_names(legnr)=titles(run);
        end
    end
    
    xlim(x_range);
    %ylim(y_range);
    set(gca,'FontSize',legend_size);
    set(gca,'TickLabelInterpreter','latex');
    leg=legend(legend_names,'Location',leg_location);
    set(leg,'Interpreter','latex');
    set(leg,'Box','off');
    set(leg,'FontSize',legend_size);
    %set(leg,'Interpreter','latex');
        
        %% Save figure
    if (save_fig == 1)
        newStr=strcat(outputdir,'/','Timeserie~',file,'~',var(col));
        if runmean > 1
            newStr=strcat(newStr,'~(',num2str(runmean),'~year~running~mean)');
        end
        newStr = strrep(newStr,'~',' ');
        print(fig,newStr{1},'-dpng',['-r',num2str(rez)],'-opengl') %save file 
    end

    nr=nr+1;
end
close all
end



%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%%
%%%      load_LPJG_data()
%%%
function [header, data] = load_LPJG_data(file)
% HDRLOAD Load data from an ASCII file containing a text header.
%     [header, data] = HDRLOAD('filename.ext') reads a data file
%     called 'filename.ext', which contains a text header.  There
%     is no default extension; any extensions must be explicitly
%     supplied.
%
%     The first output, HEADER, is the header information,
%     returned as a text array.
%     The second output, DATA, is the data matrix.  This data
%     matrix has the same dimensions as the data in the file, one
%     row per line of ASCII data in the file.  If the data is not
%     regularly spaced (i.e., each line of ASCII data does not
%     contain the same number of points), the data is returned as
%     a column vector.
%
%     Limitations:  No line of the text header can begin with
%     a number.  Only one header and data set will be read,
%     and the header must come before the data.
%
%     See also LOAD, SAVE, SPCONVERT, FSCANF, FPRINTF, STR2MAT.
%     See also the IOFUN directory.


% check number and type of arguments
if nargin < 1
    error('Function requires one input argument');
elseif ~isstr(file)
    error('Input must be a string representing a filename');
end


% Open the file.  If this returns a -1, we did not open the file
% successfully.
fid = fopen(file);
if fid==-1
    error('File not found or permission denied');
end


% Initialize loop variables
% We store the number of lines in the header, and the maximum
% length of any one line in the header.  These are used later
% in assigning the 'header' output variable.
no_lines = 0;
max_line = 0;


% We also store the number of columns in the data we read.  This
% way we can compute the size of the output based on the number
% of columns and the total number of data points.
ncols = 0;


% Finally, we initialize the data to [].
data = [];


% Start processing.
line = fgetl(fid);
if ~isstr(line)
    disp('Warning: file contains no header and no data')
end;
[data, ncols, errmsg, nxtindex] = sscanf(line, '%f');


% One slight problem, pointed out by Peter vanderWal: If the
% first character of the line is 'e', then this will scan as
% 0.00e+00. We can trap this case specifically by using the
% 'next index' output: in the case of a stripped 'e' the next
% index is one, indicating zero characters read.  See the help
% entry for 'sscanf' for more information on this output
% parameter. We loop through the file one line at a time until
% we find some data.  After that point we stop checking for
% header information. This part of the program takes most of the
% processing time, because fgetl is relatively slow (compared to
% fscanf, which we will use later).
while isempty(data)|(nxtindex==1)
    no_lines = no_lines+1;
    max_line = max([max_line, length(line)]);
    % Create unique variable to hold this line of text information.
    % Store the last-read line in this variable.
    eval(['line', num2str(no_lines), '=line;']);
    line = fgetl(fid);
    if ~isstr(line)
        disp('Warning: file contains no data')
        break
    end;
    [data, ncols, errmsg, nxtindex] = sscanf(line, '%f');
end % while


% Now that we have read in the first line of data, we can skip
% the processing that stores header information, and just read
% in the rest of the data.
data = [data; fscanf(fid, '%f')];
fclose(fid);


% Create header output from line information. The number of lines
% and the maximum line length are stored explicitly, and each
% line is stored in a unique variable using the 'eval' statement
% within the loop. Note that, if we knew a priori that the
% headers were 10 lines or less, we could use the STR2MAT
% function and save some work. First, initialize the header to an
% array of spaces.
header = setstr(' '*ones(no_lines, max_line));
for i = 1:no_lines
    varname = ['line' num2str(i)];
    % Note that we only assign this line variable to a subset of
    % this row of the header array.  We thus ensure that the matrix
    % sizes in the assignment are equal. We also consider blank
    % header lines using the following IF statement.
    if eval(['length(' varname ')~=0'])
        eval(['header(i, 1:length(' varname ')) = ' varname ';']);
    end
end % for

header = strsplit(strtrim(header));


% Resize output data, based on the number of columns (as returned
% from the sscanf of the first line of data) and the total number
% of data elements. Since the data was read in row-wise, and
% MATLAB stores data in columnwise format, we have to reverse the
% size arguments and then transpose the data.  If we read in
% irregularly spaced data, then the division we are about to do
% will not work. Therefore, we will trap the error with an EVAL
% call; if the reshape fails, we will just return the data as is.
eval('data = reshape(data, ncols, length(data)/ncols)'';', '');

end



%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%%
%%%      globalmap()
%%%
function globalmap(nr,outputdir,filename,tslice_years,file,Units,save_fig,var_name)

if (exist(strcat(outputdir,'/ref/',filename,tslice_years,'.txt'), 'file'))
    globalmapdiffall(nr,outputdir,filename,tslice_years,file,Units,save_fig,var_name);
else
    globalmapsingle(nr,outputdir,filename,tslice_years,file,Units,save_fig,var_name)
end

end



%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%%
%%%      globalmapdiffall()
%%%
function globalmapdiffall(nr,outputdir,filename,tslice_years,file,Units,save_fig,var_name)

filename1 = strcat(outputdir,'/run/',filename,tslice_years,'.txt');
filename2 = strcat(outputdir,'/ref/',filename,tslice_years,'.txt');

titlee1='run';
titlee2='ref';

coast=load('coast');

fig_width = 800;
fig_height = 350;
Northerngrid = 83.75;
% Southerngrid = -59.75;
% Westherngrid = -179.75;
% Eastherngrid = 179.75;
x_label = 'Longitude';
y_label = 'Latitude';
axis_size = 14;
% title_size = 16;
rez = 300;

tmp = parula;
std_color1=[linspace(tmp(64,1),1,5).', linspace(tmp(64,2),1,5).', linspace(tmp(64,3),1,5).'];
std_color = [parula; std_color1];

diff_color1 = [linspace(0.5,0.95,10).', zeros(10,1), zeros(10,1)];
diff_color2 = [ones(21,1), linspace(0,1,21).', linspace(0,1,21).'];
diff_color3 = [linspace(0.95,0,20).', ones(20,1), linspace(0.95,0,20).'];
diff_color4 = [zeros(10,1), linspace(0.95,0.5,10).', zeros(10,1)];
diff_color = [diff_color1; diff_color2; diff_color3; diff_color4];

[var1,dval] = load_LPJG_data(filename1);
[var2,dval] = load_LPJG_data(filename2);

clear dval;

if (strcmp('LAI', file))
    col1NS = find(strcmp('Natural_sum',var1));
    col2NS = find(strcmp('Natural_sum',var2));
end

if (nargin > 7)
    columns=find(strcmp(var_name,var1));
else
    columns=3:length(var1);
end

for col1=columns
    
    if (strcmp('LAI', file) && col1 < 15)
        nrsubplots = 6;
    else
        nrsubplots = 4;
    end
    subplotnr = 1;

    col2 = find(strcmp(var1(col1),var2));
    
    if (~isempty(col2))
        % Prepare data
        
        posandneg = 0;
        
        [xx1,yy1,zz1,grid1] = globalmapdata(filename1,var1(col1));
        [xx2,yy2,zz2,grid2] = globalmapdata(filename2,var2(col2));
        zz3 = zz1 - zz2;
        zz4 = zz3 ./ zz1 * 100;
        zz4(isinf(zz4))=0;
        
        if (strcmp('LAI', file))
            zzlog=logical(abs(((isnan(zz1+zz2))+((zz1+zz2)<0.001))-1));
        else
            zzlog=logical(~isnan(zz1+zz2));
        end
        
        zzcorrcoeff=corrcoef(zz1(zzlog),zz2(zzlog));
        
        if (nrsubplots == 6)
            [xx5,yy5,zz5,grid5] = globalmapdata(filename1,var1(col1NS));
            [xx6,yy6,zz6,grid6] = globalmapdata(filename2,var2(col2NS));
            zz7 = zz1 ./ zz5 * 100;
            zz7(isinf(zz7))=0;
            zz8 = zz2 ./ zz6 * 100;
            zz8(isinf(zz8))=0;
        end
        
        if (strcmp('LAI', file))
            range1 = prctile(sort(zz1(zz1~=0),'descend'),[2 98]);
            range2 = prctile(sort(zz2(zz2~=0),'descend'),[2 98]);
        else
            range1 = prctile(sort(zz1(:),'descend'),[2 98]);
            range2 = prctile(sort(zz2(:),'descend'),[2 98]);
        end
        range_min=min(min(range1, range2));
        range_max=max(max(range1, range2));
        if (range_min ~= 0)
            range_min=round(range_min,max(1,abs(floor(log10(abs(min(min(range1, range2))))))+1));
        else
            range_min=0;
        end
        if (range_max ~= 0)
            range_max=round(range_max,max(1,abs(floor(log10(abs(max(max(range1, range2))))))+1));
        else
            range_max=0;
        end
        if (range_min < 0 && range_max > 0 && (abs(range_max)/abs(range_min) < 200 && abs(range_max)/abs(range_min) > 0.005))
            range_max = max(abs(range_min),abs(range_max));
            range_min = -range_max;
            posandneg = 1;
        elseif (abs(range_min) > abs(range_max))
            range_max = 0;
        else
            range_min = 0;
        end
        if (strcmp('LAI', file))
            range_min = 0;
        end
        range = [range_min range_max];
        
        if (strcmp('LAI', file))
            diff_tmp = prctile(sort(zz3(zz3~=0),'descend'),[2 98]);
        else
            diff_tmp = prctile(sort(zz3(:),'descend'),[2 98]);
        end
        diff_max = max(abs(diff_tmp(1)),abs(diff_tmp(2))); 
        if (min(diff_max) ~= 0)
            diff_max=round(diff_max,max(1,abs(floor(log10(diff_max)))+1));
        else
            diff_max=0;
        end
        diff = [-diff_max diff_max];
        

        % Prepare subplots
        fig=figure(nr);
        set(fig,'Visible','off');
        set(fig,'Color','White');
        %set(fig,'Name',titlee1);
        set(fig,'Position', [100 100 fig_width*2 fig_height*(nrsubplots-2)]);

        % Paste figures on the subplots
        h(subplotnr)=subplot(nrsubplots,2,subplotnr);
        imagesc(xx1,yy1,zz1,'AlphaData',grid1);
        set(gca,'YDir','normal');
        set(gca,'TickLabelInterpreter','latex');
        caxis([range(1) range(2)]);
        if (posandneg)
            colormap(gca,diff_color);
        elseif (range(2) > 0) 
            colormap(gca,flipud(std_color)); 
        else
            colormap(gca,std_color); 
        end
        hcb=colorbar;
        hcb.Label.Interpreter = 'latex';
        xlab=xlabel(x_label);
        set(xlab,'FontSize',axis_size);
        set(xlab,'Interpreter','latex');
        ylab=ylabel(y_label);
        set(ylab,'FontSize',axis_size);
        set(ylab,'Interpreter','latex');
        newVar = strrep(var1(col1),'_','~');
        ztitle1=title(strcat('$',file,'~',titlee1,'~',newVar,'~',Units,'$'));
        set(ztitle1,'fontweight','normal');
        set(ztitle1,'FontSize',axis_size);
        set(ztitle1,'Interpreter','latex');
        geoshow(coast, 'color', 'k');
        tt=text(-179.75+5,Northerngrid-5,strcat('(',num2str(subplotnr),')'));
        set(tt,'Interpreter','latex');
        tcorrtitle=text(-179.75+5,-40,strcat(titlee1,':',titlee2));
        set(tcorrtitle,'Interpreter','latex');
        tcorr=text(-179.75+5,-50,strcat('Correlation~=~',num2str(zzcorrcoeff(1,2))));
        set(tcorr,'Interpreter','latex');
        subplotnr=subplotnr+1;

        h(subplotnr)=subplot(nrsubplots,2,subplotnr);
        imagesc(xx2,yy2,zz2,'AlphaData',grid2);
        set(gca,'YDir','normal');
        set(gca,'TickLabelInterpreter','latex');
        caxis([range(1) range(2)]);
        if (posandneg)
            colormap(gca,diff_color);
        elseif (range(2) > 0) 
            colormap(gca,flipud(std_color)); 
        else
            colormap(gca,std_color); 
        end
        hcb=colorbar;
        hcb.Label.Interpreter = 'latex';
        xlab=xlabel(x_label);
        set(xlab,'FontSize',axis_size);
        set(xlab,'Interpreter','latex');
        ylab=ylabel(y_label);
        set(ylab,'FontSize',axis_size);
        set(ylab,'Interpreter','latex');
        newVar = strrep(var2(col2),'_','~');
        ztitle2=title(strcat('$',file,'~',titlee2,'~',newVar,'~',Units,'$'));
        set(ztitle2,'fontweight','normal');
        set(ztitle2,'FontSize',axis_size);
        set(ztitle2,'Interpreter','latex');
        geoshow(coast, 'color', 'k');
        tt=text(-179.75+5,Northerngrid-5,strcat('(',num2str(subplotnr),')'));
        set(tt,'Interpreter','latex');
        tt=text(-179.75+5,Northerngrid-5,strcat('(',num2str(subplotnr),')'));
        set(tt,'Interpreter','latex');
        subplotnr=subplotnr+1;
        
        % LAI PNV fraction of total natural sum
        if (nrsubplots == 6)
            h(subplotnr)=subplot(nrsubplots,2,subplotnr);
            imagesc(xx2,yy2,zz7,'AlphaData',grid2);
            set(gca,'YDir','normal');
            set(gca,'TickLabelInterpreter','latex');
            caxis([0 100]);
            colormap(gca,flipud(hot));
            hcb=colorbar;
            hcb.Label.Interpreter = 'latex';
            xlab=xlabel(x_label);
            set(xlab,'FontSize',axis_size);
            set(xlab,'Interpreter','latex');
            ylab=ylabel(y_label);
            set(ylab,'FontSize',axis_size);
            set(ylab,'Interpreter','latex');
            newVar = strrep(var1(col1),'_','~');
            ztitle3=title(strcat('$',titlee1,'~fraction~of~total~(\frac{',newVar,'}{Natural~sum})$'));
            set(ztitle3,'fontweight','normal');
            set(ztitle3,'FontSize',axis_size);
            set(ztitle3,'Interpreter','latex');
            geoshow(coast, 'color', 'k');
            tt=text(-179.75+5,Northerngrid-5,strcat('(',num2str(subplotnr),')'));
            set(tt,'Interpreter','latex');
            subplotnr=subplotnr+1;
            
            
            h(subplotnr)=subplot(nrsubplots,2,subplotnr);
            imagesc(xx2,yy2,zz8,'AlphaData',grid2);
            set(gca,'YDir','normal');
            set(gca,'TickLabelInterpreter','latex');
            caxis([0 100]);
            colormap(gca,flipud(hot));
            hcb=colorbar;
            hcb.Label.Interpreter = 'latex';
            xlab=xlabel(x_label);
            set(xlab,'FontSize',axis_size);
            set(xlab,'Interpreter','latex');
            ylab=ylabel(y_label);
            set(ylab,'FontSize',axis_size);
            set(ylab,'Interpreter','latex');
            newVar = strrep(var2(col2),'_','~');
            ztitle4=title(strcat('$',titlee2,'~fraction~of~total~(\frac{',newVar,'}{Natural~sum})$'));
            set(ztitle4,'fontweight','normal');
            set(ztitle4,'FontSize',axis_size);
            set(ztitle4,'Interpreter','latex');
            geoshow(coast, 'color', 'k');
            tt=text(-179.75+5,Northerngrid-5,strcat('(',num2str(subplotnr),')'));
            set(tt,'Interpreter','latex');
            subplotnr=subplotnr+1;
        end

        % difference plot
        h(subplotnr)=subplot(nrsubplots,2,subplotnr);
        imagesc(xx2,yy2,zz3,'AlphaData',grid2);
        set(gca,'YDir','normal');
        set(gca,'TickLabelInterpreter','latex');
        caxis([diff(1) diff(2)]);
        colormap(gca,diff_color);
        hcb=colorbar;
        hcb.Label.Interpreter = 'latex';
        xlab=xlabel(x_label);
        set(xlab,'FontSize',axis_size);
        set(xlab,'Interpreter','latex');
        ylab=ylabel(y_label);
        set(ylab,'FontSize',axis_size);
        set(ylab,'Interpreter','latex');
        newVar = strrep(var1(col1),'_','~');
        diff_title=strcat('$',file,'~',newVar,'~differences~(',titlee1,'-',titlee2,')~');
        ztitle5=title(strcat(diff_title,Units,'$'));
        set(ztitle5,'fontweight','normal');
        set(ztitle5,'FontSize',axis_size);
        set(ztitle5,'Interpreter','latex');
        geoshow(coast, 'color', 'k');
        tt=text(-179.75+5,Northerngrid-5,strcat('(',num2str(subplotnr),')'));
        set(tt,'Interpreter','latex');
        subplotnr=subplotnr+1;
        
        
        h(subplotnr)=subplot(nrsubplots,2,subplotnr);
        grid4=zz4.*0+1;
        imagesc(xx2,yy2,zz4,'AlphaData',grid4);
        set(gca,'YDir','normal');
        set(gca,'TickLabelInterpreter','latex');
        caxis([-200 200]);
        colormap(gca,diff_color);
        hcb=colorbar;
        hcb.Label.Interpreter = 'latex';
        xlab=xlabel(x_label);
        set(xlab,'FontSize',axis_size);
        set(xlab,'Interpreter','latex');
        ylab=ylabel(y_label);
        set(ylab,'FontSize',axis_size);
        set(ylab,'Interpreter','latex');
        newVar = strrep(var1(col1),'_','~');
        ztitle6=title(strcat('$',file,'~',newVar,'~differences~(\frac{',titlee1,'-',titlee2,'}{',titlee1,'})~[\%]','$'));
        set(ztitle6,'Interpreter','latex');
        set(ztitle6,'fontweight','normal');
        set(ztitle6,'FontSize',axis_size);
        geoshow(coast, 'color', 'k');
        tt=text(-179.75+5,Northerngrid-5,strcat('(',num2str(subplotnr),')'));
        set(tt,'Interpreter','latex');
        subplotnr=subplotnr+1;
        
        if (nrsubplots == 4)
            set(h(1),'Position',[0.05,0.55,0.4,0.4]);
            set(h(2),'Position',[0.55,0.55,0.4,0.4]);
            set(h(3),'Position',[0.05,0.05,0.4,0.4]);
            set(h(4),'Position',[0.55,0.05,0.4,0.4]);
        else
            set(h(1),'Position',[0.05,0.73,0.4,0.24]);
            set(h(2),'Position',[0.55,0.73,0.4,0.24]);
            set(h(3),'Position',[0.05,0.41,0.4,0.24]);
            set(h(4),'Position',[0.55,0.41,0.4,0.24]);
            set(h(5),'Position',[0.05,0.08,0.4,0.24]);
            set(h(6),'Position',[0.55,0.08,0.4,0.24]);
        end

        %% Save figure
        if (save_fig == 1)
            newStr = strcat(outputdir,'/',file,'~',var1(col1),'~differences~between~',titlee1,'~and~',titlee2,'~(',tslice_years,')');
            newStr = strrep(newStr,'to','~to~');
            newStr = strrep(newStr,'~',' ');
            print(fig,newStr{1},'-dpng',['-r',num2str(rez)],'-opengl') %save file 
        end

        nr=nr+1;
    end
end
close all
end



%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%%
%%%      globalmapsingle()
%%%
function fig = globalmapsingle(nr,outputdir,filename,tslice_years,file,Units,save_fig,var_name)

filename = strcat(outputdir,'/run/',filename,tslice_years,'.txt');

Northerngrid = 83.75;
fig_width = 800;
fig_height = 350;
x_label = 'Longitude';
y_label = 'Latitude';
axis_size = 14;
rez = 300;

coast=load('coast');

tmp = parula;
std_color1=[linspace(tmp(64,1),1,5).', linspace(tmp(64,2),1,5).', linspace(tmp(64,3),1,5).'];
std_color = [parula; std_color1];

diff_color1 = [linspace(0.5,0.95,10).', zeros(10,1), zeros(10,1)];
diff_color2 = [ones(21,1), linspace(0,1,21).', linspace(0,1,21).'];
diff_color3 = [linspace(0.95,0,20).', ones(20,1), linspace(0.95,0,20).'];
diff_color4 = [zeros(10,1), linspace(0.95,0.5,10).', zeros(10,1)];
diff_color = [diff_color1; diff_color2; diff_color3; diff_color4];


[var,dva] = load_LPJG_data(filename);
clear dval;

% Plot figure
if (nargin > 5)
    columns=find(strcmp(var_name,var));
else
    columns=3:length(var1);
end

for col=columns
    
    posandneg = 0;
    
    [xx,yy,zz,grid] = globalmapdata(filename,var(col));
    
    if (strcmp('LAI', file))
        range = prctile(sort(zz(zz~=0),'descend'),[2 98]);
    else
        range = prctile(sort(zz(:),'descend'),[2 98]);
    end
    range_min=min(range);
    range_max=max(range);
    if (range_min ~= 0)
        range_min=round(range_min,max(1,abs(floor(log10(abs(min(range)))))+1));
    else
        range_min=0;
    end
    if (range_max ~= 0)
        range_max=round(range_max,max(1,abs(floor(log10(abs(max(range)))))+1));
    else
        range_max=0;
    end
    if (range_min < 0 && range_max > 0 && (abs(range_max)/abs(range_min) < 200 && abs(range_max)/abs(range_min) > 0.005))
        range_max = max(abs(range_min),abs(range_max));
        range_min = -range_max;
        posandneg = 1;
    elseif (abs(range_min) > abs(range_max))
        range_max = 0;
    else
        range_min = 0;
    end
    if (strcmp('LAI', file))
        range_min = 0;
    end
    range = [range_min range_max];
        
    fig=figure(nr);
    set(fig,'Visible','off');
    set(fig,'Color','White');
    %set(fig,'Name',file);
    set(fig,'Position', [100 100 fig_width fig_height]);
    imagesc(xx,yy,zz,'AlphaData',grid);
    set(gca,'YDir','normal');
    set(gca,'TickLabelInterpreter','latex');
    caxis([range(1) range(2)]);
    if (posandneg)
        colormap(gca,diff_color);
    elseif (range(2) > 0) 
        colormap(gca,flipud(std_color)); 
    else
        colormap(gca,std_color); 
    end
    hcb=colorbar;
    hcb.Label.Interpreter = 'latex';
    xlab=xlabel(x_label);
    set(xlab,'FontSize',axis_size);
    set(xlab,'Interpreter','latex');
    ylab=ylabel(y_label);
    set(ylab,'FontSize',axis_size);
    set(ylab,'Interpreter','latex');
    newVar = strrep(var(col),'_','~');
    ztitle=title(strcat('$',file,'~',newVar,'~',Units,'$'));
    set(ztitle,'fontweight','normal');
    set(ztitle,'FontSize',axis_size);
    set(ztitle,'Interpreter','latex');
    geoshow(coast, 'color', 'k');
    %tt=text(-179.75+5,Northerngrid-5,'(1)');
    %set(tt,'Interpreter','latex');

    %% Save figure
    if (save_fig == 1)
        newStr=strcat(outputdir,'/',file,'~',var(col),'~(',tslice_years,').png');
        newStr = strrep(newStr,'to','~to~');
        newStr = strrep(newStr,'~',' ');
        print(fig,newStr{1},'-dpng',['-r',num2str(rez)],'-opengl') %save file 
    end
end

end



%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%%
%%%      globalmapdata()
%%%
function [xx,yy,zz,grid] = globalmapdata(filename,plot_col)

gridcellsize = 0.5;
Northerngrid = 83.75;
Southerngrid = -59.75;

%% Map grid 0.5 * 0.5'

[grid_var,grid_data] = load_LPJG_data('gridlist_global.txt');

lat_col = find(strcmp('lat', lower(grid_var)));
lon_col = find(strcmp('lon', lower(grid_var)));
[xi,yi] = meshgrid(-179.75:gridcellsize:179.75, Southerngrid:gridcellsize:Northerngrid); %-South -55 -North 83.5 -West -175
grid=NaN(size(xi));

% Create grid
for i=1:length(grid_data(:,1))
    x_point=(grid_data(i,lon_col)+179.75)/gridcellsize+1;
    y_point=(grid_data(i,lat_col)-Southerngrid)/gridcellsize+1;
    grid(y_point,x_point)=1;
end

if sum(sum(~isnan(grid))) < length(grid_data(:,1))
    grid_nr = sum(sum(~isnan(grid)));
    error('Grid is not correct!');
end

% Lat and Lon
xx=-179.75:gridcellsize:179.75;
yy=Southerngrid:gridcellsize:Northerngrid;

[var,data] = load_LPJG_data(filename);

if (length(data) ~= 59191)
    T255 = 1;
else
    T255 = 0;
end

lat_col = find(strcmp('Lat', var));
lon_col = find(strcmp('Lon', var));
plot_col = find(strcmp(plot_col, var));


%% Determine data

if (T255 == 0)
    % CRU data with 0.5' resolution
    zz=NaN(size(xi));
    for i=1:length(data(:,1))
        x_point=(data(i,lon_col)+179.75)/gridcellsize+1;
        y_point=(data(i,lat_col)-Southerngrid)/gridcellsize+1;
        zz(y_point,x_point)=data(i,plot_col);
    end
else
    % Other grid (T255 and so)
    zz = griddata(data(:,lon_col),data(:,lat_col),data(:,plot_col),xi,yi,'nearest') .* grid;
end

end
