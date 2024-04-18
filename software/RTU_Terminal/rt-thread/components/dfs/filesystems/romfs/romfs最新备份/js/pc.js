     // $(function(){
        //     $(".menu > ul").eq(0).show();
        //     $(".menu h3").click(function(){
        //         $(this).next().stop().slideToggle();
        //         $(this).siblings().next("ul").stop().slideUp();
        //     });
        //     $(".menu > ul > li > a").click(function(){
        //     	var index=$(this).index();
        //         $(this).addClass("selected").parent().siblings().find("a").removeClass("selected");
        //         $(this).addClass("active").parent().siblings().find("a").removeClass("active");

        //     })
        // });
		$(document).ready(function(){

			function myfunction(li,li_a,menu_tab){
				li.click(function(){
				var index=$(this).index();
				menu_tab.eq(index).addClass("active").siblings().removeClass("active");
				li_a.removeClass("selected");
				li_a.eq(index).addClass("selected").siblings().removeClass("selected");
				$(".menuf .tab").css("display","none")
			});
			$(".menu h3").click(function(){
				$(".menuf .tab").css("display","none")
			})
			$(".tu12").click(function(){
				$(".menuf .tab").css("display","block")
			})
			}
			myfunction($(".container .menu .ulmenu1 li"),$(".container .ulmenu1 li a"),$(".container .content .menu1 .tab"));
			myfunction($(".container .menu .ulmenuz li"),$(".container .ulmenuz li a"),$(".container .content .menuz .tab"));
			myfunction($(".container .menu .ulmenu2 li"),$(".container .ulmenu2 li a"),$(".container .content .menu2 .tab"));
			myfunction($(".container .menu .ulmenu3 li"),$(".container .ulmenu3 li a"),$(".container .content .menu3 .tab"));
			myfunction($(".container .menu .ulmenu4 li"),$(".container .ulmenu4 li a"),$(".container .content .menu4 .tab"));
			myfunction($(".container .menu .ulmenu5 li"),$(".container .ulmenu5 li a"),$(".container .content .menu5 .tab"));
			myfunction($(".container .menu .ulmenu6 li"),$(".container .ulmenu6 li a"),$(".container .content .menu6 .tab"));
			myfunction($(".container .menu .ulmenu7 li"),$(".container .ulmenu7 li a"),$(".container .content .menu7 .tab"));
			myfunction($(".container .menu .ulmenu8 li"),$(".container .ulmenu8 li a"),$(".container .content .menu8 .tab"));
			myfunction($(".container .menu .ulmenu9 li"),$(".container .ulmenu9 li a"),$(".container .content .menu9 .tab"));
			myfunction($(".container .menu .ulmenub li"),$(".container .ulmenub li a"),$(".container .content .menub .tab"));
			myfunction($(".container .menu .ulmenue li"),$(".container .ulmenue li a"),$(".container .content .menue .tab"));
			myfunction($(".container .menu .ulmenua li"),$(".container .ulmenua li a"),$(".container .content .menua .tab"));
			myfunction($(".container .menu .ulmenuc li"),$(".container .ulmenuc li a"),$(".container .content .menuc .tab"));
			myfunction($(".container .menu .ulmenud li"),$(".container .ulmenud li a"),$(".container .content .menud .tab"));
			myfunction($(".container .menu .ulmenuf li"),$(".container .ulmenuf li a"),$(".container .content .menuf .tab"));



			// var li1=$(".container .menu ul li");
			// var lia=$(".container .menu ul li a");
			// var tab1=$(".container .content .menu1 .tab ");

			// li1.click(function(){
			// 	var index=$(this).index();

			// 	tab1.eq(index).addClass("active").siblings().removeClass("active");
			// 	lia.removeClass("selected");
			// 	lia.eq(index).addClass("selected").siblings().removeClass("selected");
			// });

			$(function(){            //ul/li的折叠效果
				$(".menu > ul").eq(0).show();
				 $(".menu h3").click(function(){
				 		//找menu对应的tab
				 		$(".menu_tab > div").removeClass("active");

				 		var val=($(this).next().attr('class'));
				 		var menu_value=(val.substring(val.length-1));
				 		$(".container .content .menu"+menu_value+" .tab:first-child").addClass("active");
				 		$(".container .menu .ulmenu"+menu_value+" li>a").removeClass("selected");
				 		$(".container .menu .ulmenu"+menu_value+" li a").eq(0).addClass("selected");//默认设置为被选中状态
				 		

				 		// $("."+"val").child().child().("selected")
				 		
				 			//这是ul收缩效果
				            $(this).next().stop().slideToggle();
				            $(this).siblings().next("ul").stop().slideUp();
							
				           // if($(".container .ulmenu"+menu_value+" li ").find("a").attr("class")==="selected"){
				           // 		$(".container .content .menu"+menu_value+" .tab:first-child")
				           // }
			            });

			});
			
			$(function(){   // 导航 >
				 $(".container .menu > h3").click(function(){

				 	$(".container .content .A1").empty().text($(this).text());
				 	
				 });
			});
		});




//下拉菜单
        var flag=0;
        var oDiv0=document.getElementById("box0");
        var oDiv1=document.getElementById("box1");
        var oDiv=document.getElementById("items");
        oDiv1.onclick=function(){
              if(flag==0){
                       oDiv.style.display="block"; 
                                   flag = 1;
               }
               else{
                                   flag=0; 
                                oDiv.style.display="none"; 
                                getCheckedValues();
                         }

        } 
        
        oDiv0.onclick=function(){
            if(flag!=0){
                                   flag=0; 
                                oDiv.style.display="none"; 
                                getCheckedValues();
                         }

        } 
        /*
         * 以下是用于当鼠标离开列表时，列表消失 
          oDiv.onmouseleave = function(){
                flag=0; this.style.display="none"; getCheckedValues();        
        } */
        function getCheckedValues(){
                
                var values = document.getElementsByName("test");
                
                var selectValue="";
                
                for(var i=0;i<values.length;i++){
                        if(values[i].checked){ 
                                selectValue += values[i].value +" | ";
                        }
                }         
                document.getElementById("box0").innerText=selectValue;
        } 



//下拉菜单2
 var flag=0;
        var oDiv00=document.getElementById("box00");
        var oDiv11=document.getElementById("box11");
        var oDivv=document.getElementById("itemss");
        oDiv11.onclick=function(){
              if(flag==0){
                       oDivv.style.display="block"; 
                                   flag = 1;
               }
               else{
                                   flag=0; 
                                oDiv.style.display="none"; 
                                getCheckedValues();
                         }

        } 
        
        oDiv00.onclick=function(){
            if(flag!=0){
                                   flag=0; 
                                oDiv.style.display="none"; 
                                getCheckedValues();
                         }

        } 
        /*
         * 以下是用于当鼠标离开列表时，列表消失 
          oDiv.onmouseleave = function(){
                flag=0; this.style.display="none"; getCheckedValues();        
        } */
        function getCheckedValues(){
                
                var values = document.getElementsByName("test");
                
                var selectValue="";
                
                for(var i=0;i<values.length;i++){
                        if(values[i].checked){ 
                                selectValue += values[i].value +" | ";
                        }
                }         
                document.getElementById("box00").innerText=selectValue;
        } 
		
		
		
		
		(function($){
	if(typeof($.fn.lc_switch) != 'undefined') {return false;} // prevent dmultiple scripts inits
	
	$.fn.lc_switch = function(on_text, off_text) {

		// destruct
		$.fn.lcs_destroy = function() {
			
			$(this).each(function() {
                var $wrap = $(this).parents('.lcs_wrap');
				
				$wrap.children().not('input').remove();
				$(this).unwrap();
            });
			
			return true;
		};	

		
		// set to ON
		$.fn.lcs_on = function() {
			
			$(this).each(function() {
                var $wrap = $(this).parents('.lcs_wrap');
				var $input = $wrap.find('input');
				
				if(typeof($.fn.prop) == 'function') {
					$wrap.find('input').prop('checked', true);
				} else {
					$wrap.find('input').attr('checked', true);
				}
				
				$wrap.find('input').trigger('lcs-on');
				$wrap.find('input').trigger('lcs-statuschange');
				$wrap.find('.lcs_switch').removeClass('lcs_off').addClass('lcs_on');
				
				// if radio - disable other ones 
				if( $wrap.find('.lcs_switch').hasClass('lcs_radio_switch') ) {
					var f_name = $input.attr('name');
					$wrap.parents('form').find('input[name='+f_name+']').not($input).lcs_off();	
				}
            });
			
			return true;
		};	
		
		
		// set to OFF
		$.fn.lcs_off = function() {
			
			$(this).each(function() {
                var $wrap = $(this).parents('.lcs_wrap');

				if(typeof($.fn.prop) == 'function') {
					$wrap.find('input').prop('checked', false);
				} else {
					$wrap.find('input').attr('checked', false);
				}
				
				$wrap.find('input').trigger('lcs-off');
				$wrap.find('input').trigger('lcs-statuschange');
				$wrap.find('.lcs_switch').removeClass('lcs_on').addClass('lcs_off');
            });
			
			return true;
		};	
		
		
		// construct
		return this.each(function(){
			
			// check against double init
			if( !$(this).parent().hasClass('lcs_wrap') ) {
			
				// default texts
				var ckd_on_txt = (typeof(on_text) == 'undefined') ? '打开' : on_text;
				var ckd_off_txt = (typeof(off_text) == 'undefined') ? '关闭' : off_text;
			   
			   // labels structure
				var on_label = (ckd_on_txt) ? '<div class="lcs_label lcs_label_on">'+ ckd_on_txt +'</div>' : '';
				var off_label = (ckd_off_txt) ? '<div class="lcs_label lcs_label_off">'+ ckd_off_txt +'</div>' : '';
				
				
				// default states
				var disabled 	= ($(this).is(':disabled')) ? true: false;
				var active 		= ($(this).is(':checked')) ? true : false;
				
				var status_classes = '';
				status_classes += (active) ? ' lcs_on' : ' lcs_off'; 
				if(disabled) {status_classes += ' lcs_disabled';} 
			   
			   
				// wrap and append
				var structure = 
				'<div class="lcs_switch '+status_classes+'">' +
					'<div class="lcs_cursor"></div>' +
					on_label + off_label +
				'</div>';
			   
				if( $(this).is(':input') && ($(this).attr('type') == 'checkbox' || $(this).attr('type') == 'radio') ) {
					
					$(this).wrap('<div class="lcs_wrap"></div>');
					$(this).parent().append(structure);
					
					$(this).parent().find('.lcs_switch').addClass('lcs_'+ $(this).attr('type') +'_switch');
				}
			}
        });
	};	
	
	
	
	// handlers
	$(document).ready(function() {
		
		// on click
		$(document).delegate('.lcs_switch:not(.lcs_disabled)', 'click tap', function(e) {

			if( $(this).hasClass('lcs_on') ) {
				if( !$(this).hasClass('lcs_radio_switch') ) { // not for radio
					$(this).lcs_off();
				}
			} else {
				$(this).lcs_on();	
			}
		});
		
		
		// on checkbox status change
		$(document).delegate('.lcs_wrap input', 'change', function() {
			if( $(this).is(':checked') ) {
				$(this).lcs_on();
			} else {
				$(this).lcs_off();	
			}	
		});
		
	});
	
})(jQuery);
