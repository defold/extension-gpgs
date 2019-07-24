<ul>
{% for field in include.fields %}
<li>
	<strong>{{ field.name }}</strong>
	{% if field.optional %}
    	(optional)
	{% endif %}
	<code>{{ field.type }}</code> - {{ field.desc | markdownify }}
</li>
{% endfor %}
</ul>
