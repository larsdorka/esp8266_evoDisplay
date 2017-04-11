// var button = document.getElementById('btn_test');
// button.addEventListener("click", function () {
//     var title = document.getElementById('title-id');
//     title.innerHTML = 'Button has been clicked!';
// });

$(function () {
    $('#myTabs a[href="#static"]').click(function (e) {
        e.preventDefault();
        $(this).tab('show');
    });
});

$(function () {
    $('#myTabs a[href="#apidoc"]').click(function (e) {
        e.preventDefault();
        $(this).tab('show');
        $.ajax({
            url: "api",
            dataType: 'text',
            async: true,
            statusCode: {
                200: function (response) {
                    $("#apidoctextarea").val(response);
                }
            },
            error: function () {
                $("#apidoctextarea").val('error');
            }
        });
    });
});

$(function () {
    $('#colorModeSelector').change(function (e) {
        if (e.currentTarget.value != 'content')
            $('#colorContentInput').attr('disabled', 'true');
        else
            $('#colorContentInput').removeAttr('disabled');

    });
});

$(function () {
    $.ajaxSetup({"contentType": "text/json"});
});

$(function () {
    $("#btn_send").click(
        function () {
            var myObj = {};
            var myColor = {};
            var myText = {};
            myColor["content"] = $("#colorContentInput").val();
            myColor["mode"] = $("#colorModeSelector").val();
            myText["content"] = $("#textContentInput").val();
            myText["mode"] = $("#textModeSelector").val();
            myObj["color"] = myColor;
            myObj["text"] = myText;
            var myJson = JSON.stringify(myObj);
            $.post("api/setdisplay", myJson, function () {
            });
        });
});

$(function () {
    $("#btn_clear").click(
        function () {
            $("#colorContentInput").val("");
            $("#colorModeSelector").val("content");
            $("#textContentInput").val("");
            $("#textModeSelector").val("0");
            $.get("api/clear", function () {
            });
        });
});
